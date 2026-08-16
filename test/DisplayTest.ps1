<#
.SYNOPSIS
    Feature test catalogue for the OpenKNX DeviceDisplay, driven over the ddc console.

.DESCRIPTION
    Detects the device (RP2040/RP2350 vs ESP32) from its own console output, runs the catalogue
    and writes a Markdown report containing every screen it saw. Two jobs, one run: regression
    test and living menu documentation.

    Screens are compared by framebuffer fingerprint, so "it changed" / "it did not change" is a
    fact rather than an impression.

    Gestures are HELD for real: press -> wait -> release. A back-to-back press/release can never
    fire a hold action, because the gesture confirm bar only fills while loop() ticks pass.

.PARAMETER Port
    Serial port. macOS/Linux: /dev/cu.usbmodemXXXX  Windows: COM5

.PARAMETER Report
    Markdown report path. Default: ./menu-report.md

.EXAMPLE
    ./DisplayTest.ps1 -Port /dev/cu.usbmodem84101
    ./DisplayTest.ps1 -Port COM5 -Report doc/menu-system/60-menu-screens.md

.NOTES
    Adding a test = one entry in $Tests. Nothing else. See test/README.md.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Port,
    [string]$Report = "./menu-report.md",
    [int]$Baud = 115200
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# ─────────────────────────────────────────────────────────────── device plumbing

class Device {
    [System.IO.Ports.SerialPort]$Sp
    [string]$Name = '?'
    [string]$Chip = '?'
    [string]$Build = '?'
    [string]$StackLabel = '?'   # Core0 (RP) vs Loop0 (ESP) -- tells the platforms apart

    Device([string]$port, [int]$baud) {
        $this.Sp = New-Object System.IO.Ports.SerialPort $port, $baud, 'None', 8, 1
        $this.Sp.WriteTimeout = 2000
        # USB-CDC: DTR is the "a terminal is attached" signal. Without it the firmware sends
        # nothing at all -- this is not a reset line here. Same as Upload-Firmware-Generic.ps1.
        $this.Sp.DtrEnable = $true
        $this.Sp.Encoding = [System.Text.Encoding]::UTF8
        $this.Sp.NewLine = "`r`n"     # the console wants CRLF; WriteLine defaults to LF only
        $this.Sp.Open()
        Start-Sleep -Milliseconds 500
        $this.Sp.Write("`r`n")        # clear any half-typed prompt state
        Start-Sleep -Milliseconds 800
        $this.Sp.DiscardInBuffer()
    }

    [string] Cmd([string]$c, [double]$wait) {
        $this.Sp.DiscardInBuffer()
        $this.Sp.WriteLine($c)
        $buf = [System.Text.StringBuilder]::new()
        $deadline = (Get-Date).AddSeconds($wait)
        while ((Get-Date) -lt $deadline) {
            Start-Sleep -Milliseconds 60
            if ($this.Sp.BytesToRead -gt 0) {
                [void]$buf.Append($this.Sp.ReadExisting())
                $deadline = (Get-Date).AddMilliseconds(400)       # extend while data flows
            }
        }
        # strip ANSI + CR so greps and hashes are stable
        return ($buf.ToString() -replace "`e\[[0-9;]*[A-Za-z]", '') -replace "`r", ''
    }

    [void] Key([string]$k)          { [void]$this.Cmd("ddc key $k", 0.4) }
    # Deterministic reset -- no key sequence whose effect depends on where the UI already is.
    [void] Home()                   { [void]$this.Cmd('ddc home', 0.6); Start-Sleep -Milliseconds 400 }
    [void] OpenMenu()               { $this.Home(); $this.Key('ok') }

    # Real hold: the confirm bar only advances while loop() ticks pass between the two events.
    [void] Hold([string]$k, [double]$seconds) {
        [void]$this.Cmd("ddc key $k press", 0.3)
        Start-Sleep -Seconds $seconds
        [void]$this.Cmd("ddc key $k release", 0.5)
    }

    # Returns @{ Fp; Lit; Art } -- Art is the half-block rendering for the report.
    [hashtable] Screen() {
        $out = $this.Cmd('ddc screenshot ascii', 1.5)
        $lines = $out -split "`n" | Where-Object { $_.Length -gt 100 -and $_ -notmatch '\$' }
        $art = ($lines -join "`n")
        $md5 = [System.Security.Cryptography.MD5]::Create()
        $hash = $md5.ComputeHash([Text.Encoding]::UTF8.GetBytes($art))
        return @{
            Fp  = ([BitConverter]::ToString($hash) -replace '-', '').Substring(0, 8).ToLower()
            Lit = @($art.ToCharArray() | Where-Object { $_ -ne ' ' -and $_ -ne "`n" }).Count
            Art = $art
        }
    }

    [void] Identify() {
        $i = $this.Cmd('i', 3.0)
        if ($i -match 'Name:\s+(OpenKNX[^\r\n]+)') { $this.Name = $Matches[1].Trim() }
        $v = $this.Cmd('v', 2.5)
        if ($v -match 'Buildtime:\s+(\S+ \S+)') { $this.Build = $Matches[1] }
        # The firmware prints Core0 on RP2040/RP2350 and Loop0 on ESP32 -- the device tells us
        # what it is, no guessing from port names.
        $m = $this.Cmd('m', 1.5)
        if ($m -match '(Core0|Loop0):') {
            $this.StackLabel = $Matches[1]
            $this.Chip = if ($Matches[1] -eq 'Core0') { 'RP2040/RP2350' } else { 'ESP32' }
        }
    }

    [void] Close() { if ($this.Sp.IsOpen) { $this.Sp.Close() } }
}

# ─────────────────────────────────────────────────────────────── test scaffolding

$script:Results  = @()
$script:Screens  = @()   # everything we saw -> the report

function Add-Screen([string]$title, [hashtable]$s) {
    $script:Screens += @{ Title = $title; Fp = $s.Fp; Lit = $s.Lit; Art = $s.Art }
}

function Add-Result([string]$name, [bool]$ok, [string]$detail = '') {
    $script:Results += @{ Name = $name; Ok = $ok; Detail = $detail }
    $tag = if ($ok) { 'PASS' } else { 'FAIL' }
    $col = if ($ok) { 'Green' } else { 'Red' }
    Write-Host ("  [{0}] {1,-46} {2}" -f $tag, $name, $detail) -ForegroundColor $col
}

# ─────────────────────────────────────────────────────────────── the catalogue
#
# One entry = one check. Run gets the Device and returns @{ Ok; Detail }.
# Optional Section groups the console output and the report.

$Tests = @(
    # ── Konsole ────────────────────────────────────────────────────────────────
    @{ Section = 'Konsole'; Name = 'v liefert Firmware-Version'; Run = { param($d)
        @{ Ok = ($d.Cmd('v', 2.5) -match 'This Firmware') } } }

    @{ Name = 'm liefert Speicher und Stack'; Run = { param($d)
        $m = $d.Cmd('m', 1.5)
        @{ Ok = ($m -match 'Free memory' -and $m -match 'Free stack') } } }

    @{ Name = 'Stack-Marke positiv (kein Ueberlauf)'; Run = { param($d)
        $m = $d.Cmd('m', 1.5)
        if ($m -notmatch '(Core0|Loop0):\s+(-?\d+) bytes') { return @{ Ok = $false; Detail = 'nicht lesbar' } }
        $v = [int]$Matches[2]
        @{ Ok = ($v -gt 0); Detail = "$($Matches[1]): $v bytes" } } }

    @{ Name = 'ddc l listet die Widget-Queue'; Run = { param($d)
        $l = $d.Cmd('ddc l', 2.0)
        @{ Ok = ($l -match 'Widget Queue' -and $l -match 'Menu') } } }

    # ── Menue ─────────────────────────────────────────────────────────────────
    @{ Section = 'Menue'; Name = 'Home-Screen zeichnet etwas'; Run = { param($d)
        $d.Home(); $s = $d.Screen(); Add-Screen 'Home-Screen' $s
        @{ Ok = ($s.Lit -gt 50); Detail = "lit=$($s.Lit)" } } }

    @{ Name = 'OK oeffnet das Menue'; Run = { param($d)
        $d.Home(); $h = $d.Screen()
        $d.Key('ok'); $s = $d.Screen(); Add-Screen 'Wurzelmenue' $s
        @{ Ok = ($s.Fp -ne $h.Fp); Detail = "fp=$($s.Fp)" } } }

    @{ Name = 'Menue-Oeffnen ist reproduzierbar (3x)'; Run = { param($d)
        $fps = 1..3 | ForEach-Object { $d.OpenMenu(); $d.Screen().Fp }
        @{ Ok = (@($fps | Select-Object -Unique).Count -eq 1); Detail = "fp=$($fps[0])" } } }

    @{ Name = 'DOWN bewegt den Cursor'; Run = { param($d)
        $d.OpenMenu(); $a = $d.Screen().Fp; $d.Key('down'); $b = $d.Screen().Fp
        @{ Ok = ($a -ne $b) } } }

    @{ Name = 'UP kehrt zur vorigen Zeile zurueck'; Run = { param($d)
        $d.OpenMenu(); $a = $d.Screen().Fp; $d.Key('down'); $d.Key('up')
        @{ Ok = ($d.Screen().Fp -eq $a) } } }

    @{ Name = 'LEFT geht 1x zurueck ins Wurzelmenue'; Run = { param($d)
        $d.OpenMenu(); $root = $d.Screen().Fp
        $d.Key('ok'); $sub = $d.Screen(); Add-Screen 'System-Info' $sub
        $d.Key('left')
        @{ Ok = ($d.Screen().Fp -eq $root); Detail = '1 LEFT' } } }

    @{ Name = 'LEFT-halten schliesst das Menue ganz'; Run = { param($d)
        # Nach dem Schliessen laeuft die Widget-Rotation weiter -- der Screen ist also NICHT
        # zwangslaeufig derselbe wie vorher. Pruefbar ist nur: das Menue ist weg.
        $d.OpenMenu(); $menuFp = $d.Screen().Fp
        $d.Key('ok'); $subFp = $d.Screen().Fp
        [void]$d.Cmd('ddc key left long', 0.6); Start-Sleep -Milliseconds 600
        $now = $d.Screen().Fp
        @{ Ok = ($now -ne $menuFp -and $now -ne $subFp); Detail = "fp=$now" } } }

    # ── Editoren ──────────────────────────────────────────────────────────────
    @{ Section = 'Editoren'; Name = 'Helligkeit oeffnet den Slider'; Run = { param($d)
        $d.OpenMenu(); 1..2 | ForEach-Object { $d.Key('down') }   # -> Anzeige
        $d.Key('ok'); $anz = $d.Screen(); Add-Screen 'Anzeige' $anz
        $d.Key('ok'); $s = $d.Screen(); Add-Screen 'Slider: Helligkeit' $s
        @{ Ok = ($s.Fp -ne $anz.Fp); Detail = "fp=$($s.Fp)" } } }

    @{ Name = 'Bildschirmschoner oeffnet die RadioList'; Run = { param($d)
        $d.OpenMenu(); 1..2 | ForEach-Object { $d.Key('down') }
        $d.Key('ok'); $anz = $d.Screen().Fp
        1..6 | ForEach-Object { $d.Key('down') }                  # -> Bildschirmschoner
        $d.Key('ok'); $s = $d.Screen(); Add-Screen 'RadioList: Bildschirmschoner' $s
        @{ Ok = ($s.Fp -ne $anz); Detail = "fp=$($s.Fp)" } } }

    # ── Gesten (echtes Halten) ────────────────────────────────────────────────
    @{ Section = 'Gesten'; Name = 'OK 5s halten loest ProgMode aus'; Run = { param($d)
        # Wirkung pruefen, nicht Log-Text: das ProgMode-Widget wird zum aktuellen Widget.
        # Halten muss > HOLD_PHASE1_MS(1000) + GESTURE_BAR_MS(3000) sein -- 4s ist die Kante.
        $d.Home()
        $before = ($d.Cmd('ddc l', 2.0) -match 'ProgMode.*\|\s*YES')
        $d.Hold('ok', 5.0); Start-Sleep -Seconds 1
        $on = ($d.Cmd('ddc l', 2.0) -match 'ProgMode.*\|\s*YES')
        if ($on) { $d.Hold('ok', 5.0); Start-Sleep -Milliseconds 800 }   # zuruecknehmen
        @{ Ok = ($on -and -not $before); Detail = if ($on) { 'ProgMode-Widget aktiv' } else { 'Widget nicht aktiv' } } } }

    @{ Name = 'RIGHT 4s halten schreibt Screenshot auf SD'; Run = { param($d)
        $d.Home()
        $before = ([regex]::Matches($d.Cmd('sdc ls', 2.5), '\.bmp')).Count
        $d.Hold('right', 4.0); Start-Sleep -Seconds 2
        $after = ([regex]::Matches($d.Cmd('sdc ls', 2.5), '\.bmp')).Count
        @{ Ok = ($after -gt $before); Detail = "$before -> $after Dateien" } } }

    @{ Name = 'KONAMI stellt Anzeige-Defaults wieder her'; Run = { param($d)
        # Sequenz ist UP UP DOWN DOWN LEFT RIGHT LEFT RIGHT SELECT SELECT -- ZWEI SELECT
        # (DeviceDisplay.cpp::matchKonami). End-to-end: Wert verbiegen, Sequenz, Wert muss zurueck.
        $d.Home()
        [void]$d.Cmd('ddc config set brightness 3', 1.0)
        $bent = ($d.Cmd('ddc config', 2.0) -match 'brightness\s*:\s*3')
        'up','up','down','down','left','right','left','right','ok','ok' | ForEach-Object { $d.Key($_) }
        Start-Sleep -Seconds 1
        $restored = ($d.Cmd('ddc config', 2.0) -match 'brightness\s*:\s*9')
        @{ Ok = ($bent -and $restored)
           Detail = if (-not $bent) { 'Wert liess sich nicht verstellen' }
                    elseif ($restored) { 'brightness 3 -> 9 (Default)' } else { 'nicht zurueckgesetzt' } } } }

    # ── SD / Datei-Browser ────────────────────────────────────────────────────
    @{ Section = 'SD-Karte'; Name = 'SD ist gemountet'; Run = { param($d)
        @{ Ok = ($d.Cmd('sdc i', 3.0) -match 'Card Type') } } }

    @{ Name = 'Datei-Browser oeffnet'; Run = { param($d)
        $d.OpenMenu(); 1..7 | ForEach-Object { $d.Key('down') }    # -> SD-Karte
        $d.Key('ok'); $sd = $d.Screen(); Add-Screen 'SD-Karte' $sd
        $d.Key('ok'); $b = $d.Screen(); Add-Screen 'Datei-Browser' $b
        @{ Ok = ($b.Fp -ne $sd.Fp); Detail = "fp=$($b.Fp) lit=$($b.Lit)" } } }

    @{ Name = 'DOWN blaettert im Datei-Browser'; Run = { param($d)
        $d.OpenMenu(); 1..7 | ForEach-Object { $d.Key('down') }
        $d.Key('ok'); $d.Key('ok')
        $prev = $d.Screen().Fp; $moved = 0
        1..3 | ForEach-Object {
            $d.Key('down'); $fp = $d.Screen().Fp
            if ($fp -ne $prev) { $moved++ }
            $prev = $fp
        }
        @{ Ok = ($moved -eq 3); Detail = "$moved/3 Schritte bewegten den Cursor" } } }

    @{ Name = 'LEFT: Zeile 1 zuerst, dann raus (2 Schritte)'; Run = { param($d)
        # Dokumentiertes Modell (WidgetFileBrowser.cpp): nicht in Zeile 1 -> springe zu Zeile 1;
        # in Zeile 1 am SD-Root -> Browser verlassen. Nach DOWN sind also 2x LEFT korrekt.
        $d.OpenMenu(); 1..7 | ForEach-Object { $d.Key('down') }
        $d.Key('ok'); $sd = $d.Screen().Fp
        $d.Key('ok'); $d.Key('down'); $row2 = $d.Screen().Fp
        $d.Key('left'); $mid = $d.Screen().Fp     # nicht Zeile 1 -> springt auf Zeile 1
        $d.Key('left'); $out = $d.Screen().Fp     # auf Zeile 1 am Root -> raus
        @{ Ok = ($mid -ne $row2 -and $mid -ne $sd -and $out -eq $sd)
           Detail = "LEFT#1 -> Zeile 1 ($mid), LEFT#2 -> SD-Menue" } } }

    # ── Abschluss ─────────────────────────────────────────────────────────────
    @{ Section = 'Abschluss'; Name = 'Stack nach allen Tests noch positiv'; Run = { param($d)
        $m = $d.Cmd('m', 1.5)
        if ($m -notmatch '(Core0|Loop0):\s+(-?\d+) bytes') { return @{ Ok = $false } }
        @{ Ok = ([int]$Matches[2] -gt 0); Detail = "$($Matches[1]): $($Matches[2]) bytes" } } }

    @{ Name = 'Heap-Minimum hat Reserve (> 20 KiB)'; Run = { param($d)
        $m = $d.Cmd('m', 1.5)
        if ($m -notmatch 'Free memory:\s+([\d.]+) KiB \(min\. ([\d.]+) KiB\)') { return @{ Ok = $false } }
        @{ Ok = ([double]$Matches[2] -gt 20); Detail = "frei $($Matches[1]) KiB, min $($Matches[2]) KiB" } } }
)

# ─────────────────────────────────────────────────────────────── run

$dev = [Device]::new($Port, $Baud)
try {
    $dev.Identify()
    Write-Host ('=' * 78)
    Write-Host "TESTKATALOG  $($dev.Name)"
    Write-Host "Port $Port   Chip $($dev.Chip)   Build $($dev.Build)"
    Write-Host ('=' * 78)

    $section = ''
    foreach ($t in $Tests) {
        if ($t.ContainsKey('Section') -and $t.Section -ne $section) {
            $section = $t.Section
            Write-Host "`n--- $section ---"
        }
        try {
            $r = & $t.Run $dev
            Add-Result $t.Name ([bool]$r.Ok) ($(if ($r.ContainsKey('Detail')) { $r.Detail } else { '' }))
        } catch {
            Add-Result $t.Name $false "Ausnahme: $($_.Exception.Message)"
        }
    }
    $dev.Home()

    $pass = @($script:Results | Where-Object { $_.Ok }).Count
    Write-Host "`n$('=' * 78)"
    Write-Host "ERGEBNIS: $pass/$($script:Results.Count) bestanden"
    $script:Results | Where-Object { -not $_.Ok } | ForEach-Object {
        Write-Host "   FEHLGESCHLAGEN: $($_.Name) $($_.Detail)" -ForegroundColor Red
    }
    Write-Host ('=' * 78)

    # ── report: test result + every screen we saw = living menu documentation
    $md = [System.Text.StringBuilder]::new()
    [void]$md.AppendLine("# Menü-Testbericht — $($dev.Name)")
    [void]$md.AppendLine()
    [void]$md.AppendLine("| | |")
    [void]$md.AppendLine("|---|---|")
    [void]$md.AppendLine("| Gerät | $($dev.Name) |")
    [void]$md.AppendLine("| Chip | $($dev.Chip) (Stack-Marke `"$($dev.StackLabel):`") |")
    [void]$md.AppendLine("| Firmware | $($dev.Build) |")
    [void]$md.AppendLine("| Port | ``$Port`` |")
    [void]$md.AppendLine("| Ergebnis | **$pass/$($script:Results.Count)** bestanden |")
    [void]$md.AppendLine()
    [void]$md.AppendLine('## Ergebnisse')
    [void]$md.AppendLine()
    [void]$md.AppendLine('| | Test | Detail |')
    [void]$md.AppendLine('|---|---|---|')
    foreach ($r in $script:Results) {
        $icon = if ($r.Ok) { '✅' } else { '❌' }
        [void]$md.AppendLine("| $icon | $($r.Name) | $($r.Detail) |")
    }
    [void]$md.AppendLine()
    [void]$md.AppendLine('## Screens')
    [void]$md.AppendLine()
    [void]$md.AppendLine('Direkt aus dem Framebuffer (128x64, Halbblock: 2 Pixelzeilen pro Zeichen).')
    [void]$md.AppendLine('Der Fingerprint ist der MD5 des Bildes — gleiche fp = pixelgleicher Screen.')
    [void]$md.AppendLine()
    foreach ($s in $script:Screens) {
        [void]$md.AppendLine("### $($s.Title)")
        [void]$md.AppendLine()
        [void]$md.AppendLine("`fp=$($s.Fp)`  ·  $($s.Lit) gesetzte Pixel")
        [void]$md.AppendLine()
        [void]$md.AppendLine('```text')
        [void]$md.AppendLine($s.Art)
        [void]$md.AppendLine('```')
        [void]$md.AppendLine()
    }
    $md.ToString() | Set-Content -Path $Report -Encoding utf8
    Write-Host "Bericht: $Report  ($($script:Screens.Count) Screens)"
}
finally {
    $dev.Close()
}

exit ($(if (@($script:Results | Where-Object { -not $_.Ok }).Count) { 1 } else { 0 }))
