# ToDo — OFM-DeviceDisplay

| Nr. | Thema |
|---|---|
| 1 | Display-Einstellungen über die ETS – der User entscheidet, wer führt |
| 2 | Einstellungen im Web exportieren und importieren |
| 3 | „Anzeige → Zurücksetzen“ startet das Gerät neu |

---

## 1 — Display-Einstellungen über die ETS

### Was ich will

Alle Display-Einstellungen sollen auch in der ETS stehen. Der User entscheidet dort, wer das Sagen hat:

| Auswahl in der ETS | Ändern am Gerät oder im Web | Nach einem ETS-Download |
|---|---|---|
| ETS führt | geht, wirkt sofort | die ETS-Werte gelten wieder, die Änderungen vom Gerät sind weg |
| Gerät führt | geht, wirkt sofort | das Gerät behält seine Werte |
| Gesperrt | geht nicht, Gerät und Web zeigen nur an | die ETS-Werte gelten |

Es ist also in Ordnung, wenn jemand am Display die ETS-Werte überschreibt – und genauso in Ordnung, wenn
der nächste Download sie wieder zurückholt. Das muss der User selbst wählen können.

„Gesperrt“ ist das Sperren, das ich später ohnehin in der ETS haben will (Display, Tasten, was angezeigt
wird). Das ist kein eigener Mechanismus, sondern einfach die dritte Auswahl.

Solange es den ETS-Teil nicht gibt, bleibt die Konfiguration, was angezeigt wird, im Display und im Web –
wenn das Web verfügbar ist. Sonst ETS und/oder Display. Alle drei Wege sollen am Ende dieselben Werte
bearbeiten.

**Wichtig ist mir: Die Konfiguration bleibt sauber erhalten.** Kein Update darf dem User seine
Einstellungen wegnehmen.

### Was schon da ist

- Die Einstellungen liegen als ein Block im Flash, über den gemeinsamen Speicher aus OGM-Common. Der Block
  hängt an der Modulnummer, andere Module verschieben ihn nicht.
- OFM-Network macht das Prinzip schon vor: Was am Gerät für die IP eingestellt wird, liegt als lokale
  Abweichung über dem ETS-Wert (`_localOverride`).
- Das Modul bekommt mit, wenn die ETS neu programmiert: `processBeforeTablesUnload()`. Das gibt es aber nur
  auf einem programmierten Gerät.
- Eine neue Applikationsversion wirft die gespeicherten Daten nicht weg, nur eine andere
  Applikationsnummer.
- Das Display-Modul hat heute noch gar keinen ETS-Teil.

### Wie wir speichern

- Was am Gerät oder im Web geändert wird, ist der „lokale Satz“. Der Block bleibt, wie er ist, und bekommt
  hinten ein Feld dazu: „lokaler Satz gilt“.
- Gilt er nicht, zählt der ETS-Wert. Gilt er, zählt der gespeicherte Wert.
- Bei „ETS führt“ löscht der Download das Feld, und nach dem Neustart gelten die ETS-Werte.
- Das Ganze für den kompletten Satz auf einmal, nicht für jede Einstellung einzeln. Das hält es einfach.

**Regel: Der Block wird nur hinten erweitert.** Eine neue Firmware liest einen alten, kürzeren Block und
nimmt für das Fehlende den Standardwert. Eine alte Firmware ignoriert, was sie nicht kennt. Die
Formatversion ändert sich nur, wenn wir den Block wirklich umbauen. Die 16 Widget-Plätze liegen fest in
der Mitte – mehr als 16 wäre so ein Umbau.

Für jetzt reicht es, dass wir uns den Weg nicht verbauen. Genau dafür ist diese Regel da.

### Widgets von anderen Modulen und von den OAMs

Das Display-Modul weiß nicht, welche Widgets es im Produkt gibt. OGM-Common bringt zwei mit, OFM-Network
zwei, OFM-SDCard eins, das OAM sein eigenes. Die ETS muss die Liste aber kennen.

Also brauchen die OFMs und OAMs eine Möglichkeit, ihre Widgets per XML anzumelden. **Als Vorlage nehmen wir
die LED-Funktionen der Info-LEDs**, da ist genau das schon gelöst:

- OGM-Common legt den Parametertyp `PT-SLEDFunc` mit seinen Einträgen an (`Common.share.xml`).
- OFM-Network legt in seinem eigenen ETS-Teil einen Typ mit **demselben Namen** an und hängt
  „Netzwerkstatus“ und „KNX-IP-Status“ dran.
- Der Producer fügt das zu einer Liste zusammen. In der erzeugten Router-Applikation steht eine Liste mit
  allen sieben Einträgen.
- Das Produkt setzt seine Vorgaben per `op:config`.

Für Widgets genauso:

- Das Display-Modul legt den Typ „Widget“ mit seinen eigenen Einträgen an (Uhr, Zeit).
- Jedes OFM und jedes OAM, das ein Widget mitbringt, hängt seinen Eintrag in seinem eigenen ETS-Teil dran.
- In der ETS sieht das aus wie bei den LEDs: „Platz 1 … n: Widget (Auswahl), Dauer“. Damit sind
  Reihenfolge, Sichtbarkeit und Dauer in einem erledigt.
- Die Grundausstattung (Clock, Time, System-Info, KNX / BCU) ist immer vorbelegt. Das OAM kann per
  `op:config` weitere Vorgaben dazulegen, muss aber nicht.
- Der User fügt weitere Widgets aus der Liste hinzu, wenn er will.

### Woran ein Widget erkannt wird

Die OFMs haben keine Nummer, die überall gleich ist:

| Nummer | Wo | Problem |
|---|---|---|
| `openknx.addModule(n, …)` | `main.cpp` vom Produkt | wählt das Produkt: 10 ist bei uns das Display, im NeoPixel-OAM FunctionBlocks |
| `ModuleType` | ETS-Teil vom Produkt | wählt das Produkt: BASE und LOG haben beide 10 |
| `ETS_ModuleId_…` | erzeugte `knxprod.h` | nur die Position im Produkt, verschiebt sich mit jedem neuen Modul |

Eine eigene Nummerntabelle im Display-Modul will ich nicht. Das wäre die vierte Zählung, die jeder
OFM-Autor kennen müsste.

Was jedes OFM aber hat, und zwar eindeutig in ganz OpenKNX: sein **Präfix** (`BASE`, `NET`, `FTM`, `LOG`,
`NEO` …). Also:

- **Kennung = `PRÄFIX.Schlüssel`**, zum Beispiel `NET.Speed`, `BASE.SysInfo`, `SD.Card`, `DD.Clock`. Das
  OFM setzt sie da, wo es heute den Namen setzt. Keine Tabelle, keine Abstimmung.
- **Der Name ist nur noch die Beschreibung.** Den kann man ändern und übersetzen, ohne dass die
  Einstellungen weg sind.
- **Gespeichert wird wie heute** (Hash), nur aus dem Schlüssel statt aus dem Namen. Ein Widget ohne
  Schlüssel läuft weiter über seinen Namen. Gesucht wird erst nach Schlüssel, dann nach Name – so kann ein
  OFM seinen Schlüssel später nachziehen, und nichts geht verloren. Am Speicher muss dafür nichts geändert
  werden.

Offen ist nur noch die Zahl, die die ETS für einen Listeneintrag braucht. Zwei Wege, beide ohne Tabelle:

- **(a) Eine gemeinsame Liste, die Nummer kommt vom Produkt.** Das OFM schreibt `Value="%TT%01"` in seinen
  ETS-Teil, der Producer setzt die Modulnummer ein, die das Produkt vergeben hat. Die Firmware liest
  dieselbe Zahl aus einem versteckten Parameter. Ergibt eine Seite mit allen Widgets, wie bei den LEDs.
  Muss noch gezeigt werden: Ersetzt der Producer `%TT%` an der Stelle? Und was ist mit BASE und LOG und
  ihrer gemeinsamen 10?
- **(b) Keine gemeinsame Liste.** Jedes OFM bringt „anzeigen, Dauer, Platz“ für seine Widgets als eigene
  Parameter mit. Braucht gar keine geteilte Zahl, aber die Widgets sind dann in der ETS über die Seiten
  der Module verteilt.

Ich will (a), wenn es geht. Auch offen: wie viele Plätze die ETS anbietet – fest 16 oder vom Produkt
vorgegeben.

### ETS holt sich die Konfiguration vom Gerät

Die ETS liest von sich aus nichts zurück. Stellt jemand am Gerät um, zeigt das ETS-Projekt weiter den alten
Wert. Das will ich nicht nur als Hinweis im Text haben, sondern lösen:

**Ein Knopf in der Applikation: „Einstellungen vom Gerät holen“.** Die ETS fragt das Gerät nach seiner
Konfiguration und setzt die Parameter so, wie sie im Gerät stehen.

Die Bausteine gibt es schon:

- Knopf mit Skript, das mit dem Gerät redet: „Übertragen“ in `Network.share.xml` / `Network.script.js`.
- Skript, das das Gerät fragt und die Antwort in Parameter schreibt: `BASE_getUnsupportedEtsModules()` in
  `Common.script.js` („Module abgleichen“).
- Auf dem Gerät: `processFunctionProperty()` im Modul.
- Für Antworten, die nicht in ein Telegramm passen: `BASE_invokeFunctionPropertyWrapper()`.

Offen:

- Schicken wir den Block, wie er gespeichert ist (150 Byte), oder als Name und Wert? Wie gespeichert ist
  weniger Code im Gerät, dafür muss das Skript den Aufbau kennen.
- Geht der Wrapper für lange Antworten so, wie er ist?
- In unseren Produkten ist der Block „Module abgleichen“ absichtlich raus. Ob ein Knopf-Skript bei uns
  überhaupt läuft, muss erst am Gerät gezeigt werden, bevor wir darauf bauen.

### Erster Entwurf: So sieht es in der ETS aus

Aufteilung und Texte sind ein Vorschlag. Die Namen, Bereiche und Vorgaben sind die, die Gerät und Web
heute schon haben – da soll nichts doppelt erfunden werden. Die Hilfetexte sind für den User geschrieben,
der die Anlage einrichtet: kurz, ohne Du und Sie.

Im Baum der Applikation gibt es einen Eintrag **Display** mit fünf Seiten:

```
Display
 ├─ Allgemein
 ├─ Anzeige
 ├─ Widgets
 ├─ Home-Tasten
 └─ Experte
```

#### Seite „Allgemein“

```
Display
──────────────────────────────────────────────────────────────
 Einstellungen am Gerät        [ Erlaubt – ETS führt         ▼ ]
                                 Erlaubt – Gerät führt
                                 Gesperrt

 ⓘ Ein ETS-Download überschreibt, was am Gerät geändert wurde.

 Einstellungen vom Gerät holen               [ Vom Gerät holen ]
```

| Parameter | Auswahl | Vorgabe |
|---|---|---|
| Einstellungen am Gerät | Erlaubt – ETS führt / Erlaubt – Gerät führt / Gesperrt | Erlaubt – ETS führt |
| Einstellungen vom Gerät holen | Knopf | – |

Die Hinweiszeile wechselt mit der Auswahl:

- ETS führt: „Ein ETS-Download überschreibt, was am Gerät geändert wurde.“
- Gerät führt: „Was am Gerät geändert wurde, bleibt nach einem ETS-Download erhalten. Die Werte hier gelten
  nur, solange am Gerät nichts geändert wurde.“
- Gesperrt: „Am Gerät und auf der Web-Seite lässt sich nichts ändern. Es gelten die Werte aus der ETS.“

**Hilfe „Einstellungen am Gerät“**

> Legt fest, ob die Anzeige auch am Gerät selbst und auf der Web-Seite eingestellt werden darf – und
> welche Werte nach einem ETS-Download gelten.
>
> **Erlaubt – ETS führt:** Änderungen am Gerät wirken sofort. Der nächste ETS-Download setzt wieder die
> Werte aus der ETS.
>
> **Erlaubt – Gerät führt:** Änderungen am Gerät wirken sofort und bleiben auch nach einem ETS-Download
> bestehen.
>
> **Gesperrt:** Gerät und Web-Seite zeigen die Einstellungen nur an.

**Hilfe „Einstellungen vom Gerät holen“**

> Liest die Einstellungen aus, die das Gerät gerade verwendet, und trägt sie hier ein. Sinnvoll, wenn am
> Gerät oder auf der Web-Seite etwas geändert wurde und das Projekt den gleichen Stand haben soll.
>
> Das Gerät muss am Bus erreichbar sein. Die ETS überträgt dabei nichts zum Gerät.

#### Seite „Anzeige“

```
Display › Anzeige
──────────────────────────────────────────────────────────────
 Helligkeit                    [ 100 %                       ▼ ]
 Auto-Dim nach                 [  15 ] Minuten      (0 = aus)
 Dim-Level                     [ 30 %                        ▼ ]
 Bildschirmschoner             [ Clock                       ▼ ]
 Bildschirmschoner nach        [  30 ] Minuten      (0 = aus)
 Display aus nach              [  60 ] Minuten      (0 = nie)

 Invertieren                   [ ] 
 Um 180° drehen                [ ]
 Seiten auto-blättern          [x]
 Icon-Menü                     [ ]
```

| Parameter | Auswahl | Vorgabe |
|---|---|---|
| Helligkeit | 10 % … 100 % in Schritten von 10 % | 100 % |
| Auto-Dim nach | 0 … 999 Minuten, 0 = aus | 15 |
| Dim-Level | nie, 10 % … 90 % | 30 % |
| Bildschirmschoner | Clock, Cube3D, Doom, FireWorks, Life, Matrix, MatrixCl., Pong, Rain, Starfield, Aus | Clock |
| Bildschirmschoner nach | 0 … 999 Minuten, 0 = aus | 30 |
| Display aus nach | 0 … 999 Minuten, 0 = nie | 60 |
| Invertieren | ja / nein | nein |
| Um 180° drehen | ja / nein | nein |
| Seiten auto-blättern | ja / nein | ja |
| Icon-Menü | ja / nein | nein |

**Hilfe „Helligkeit“**

> Helligkeit der Anzeige im normalen Betrieb.

**Hilfe „Auto-Dim nach“ und „Dim-Level“**

> Ohne Tastendruck wird die Anzeige nach dieser Zeit dunkler. Das Dim-Level gibt an, wie hell sie dann
> noch ist; sie wird dabei nie heller als die eingestellte Helligkeit. Mit 0 Minuten oder Dim-Level „nie“
> entfällt das Abdunkeln.

**Hilfe „Bildschirmschoner“ und „Bildschirmschoner nach“**

> Ohne Tastendruck zeigt das Display nach dieser Zeit den gewählten Bildschirmschoner statt der Widgets.
> Mit 0 Minuten oder der Auswahl „Aus“ gibt es keinen Bildschirmschoner.

**Hilfe „Display aus nach“**

> Ohne Tastendruck schaltet sich das Display nach dieser Zeit aus. Mit 0 bleibt es an. Jeder Tastendruck
> schaltet es wieder ein und stellt die volle Helligkeit her.

**Hilfe „Invertieren“**

> Tauscht hell und dunkel: dunkle Schrift auf hellem Grund.

**Hilfe „Um 180° drehen“**

> Dreht das Bild auf den Kopf – für Geräte, die über Kopf eingebaut sind.

**Hilfe „Seiten auto-blättern“**

> Manche Widgets haben mehrere Seiten. Mit dieser Einstellung blättert das Display sie von selbst durch;
> die Anzeigedauer des Widgets wird dafür gleichmäßig auf die Seiten verteilt. Ohne sie wird nur mit
> ▲ und ▼ geblättert.

**Hilfe „Icon-Menü“**

> Zeigt die oberste Ebene des Gerätemenüs als Symbole statt als Liste.

#### Seite „Widgets“

So soll es laufen:

1. **Die Grundausstattung ist vorbelegt.** Clock, Time, System-Info und KNX / BCU stehen immer schon drin.
2. **Das OAM kann per XML weitere Vorgaben dazulegen – oder auch nicht.** Der IP-Router zum Beispiel sein
   eigenes Widget.
3. **Der User kann weitere Widgets aus der Liste hinzufügen – oder auch nicht.** Unter den vorbelegten
   Plätzen steht dafür ein leerer Platz „Widget hinzufügen“. Ist er belegt, erscheint der nächste.

```
Display › Widgets
──────────────────────────────────────────────────────────────
 ⓘ Das Display zeigt die Widgets der Reihe nach, von Platz 1 an.

          Widget                           Dauer
 Platz 1  [ Clock                      ▼ ] [  5 s ▼ ]   ┐
 Platz 2  [ Time                       ▼ ] [  5 s ▼ ]   │ Grundausstattung,
 Platz 3  [ System-Info                ▼ ] [  8 s ▼ ]   │ immer vorbelegt
 Platz 4  [ KNX / BCU                  ▼ ] [  8 s ▼ ]   ┘
 Platz 5  [ IP-Router                  ▼ ] [ 15 s ▼ ]   ← Vorgabe vom OAM
 Platz 6  [ – Widget hinzufügen –      ▼ ]
```

Wählt der User auf Platz 6 zum Beispiel „LAN-Speed“, kommt die Dauer dazu und Platz 7 erscheint:

```
 Platz 6  [ LAN-Speed                  ▼ ] [ 10 s ▼ ]
 Platz 7  [ – Widget hinzufügen –      ▼ ]
```

| Parameter | Auswahl | Vorgabe |
|---|---|---|
| Platz n – Widget | alle Widgets, die das Produkt und seine Module mitbringen, und „– leer –“ | Grundausstattung, danach die Vorgaben vom OAM, der Rest leer |
| Platz n – Dauer | 5 s, 8 s, 10 s, 15 s, 20 s, 30 s | die Dauer, die das Widget mitbringt |

In der Liste stehen alle Widgets, die es im Gerät gibt – beim IP-Router heute zusätzlich SD-Card,
LAN-Speed und Netzwerk. Was nicht auf einem Platz steht, wird nicht angezeigt.

So verstehe ich es bisher, und so ist es hier eingetragen: Auch ein vorbelegter Platz lässt sich
umstellen oder auf „– leer –“ setzen. Die Vorbelegung ist ein Startwert, keine Pflicht.

**Hilfe „Widgets“**

> Das Display zeigt die Widgets nacheinander an, beginnend bei Platz 1, und fängt danach wieder von vorn
> an. Die Dauer gibt an, wie lange ein Widget stehen bleibt.
>
> Die ersten Plätze sind vorbelegt. Über den freien Platz darunter lässt sich ein weiteres Widget
> hinzufügen; danach erscheint der nächste freie Platz.
>
> Ein Widget, das auf keinem Platz steht, wird nicht angezeigt. Ein leerer Platz wird übersprungen.
>
> Welche Widgets zur Auswahl stehen, hängt vom Gerät und seinen Modulen ab.

Was dafür im XML zu klären ist:

- Die Grundausstattung steht im ETS-Teil des Display-Moduls. System-Info und KNX / BCU kommen aber aus
  OGM-Common – das Display-Modul muss ihre Einträge also kennen, obwohl sie ihm nicht gehören. Hängt an der
  Frage (a) oder (b) oben.
- Das OAM legt seine Vorgaben per `op:config` auf die nächsten Plätze, so wie bei den LED-Funktionen.
- „Nächster Platz erscheint, wenn der vorige belegt ist“ ist in der ETS eine Kette aus `choose`/`when`.
- Dasselbe Widget auf zwei Plätzen: Der erste Platz zählt, oder die ETS lässt es gar nicht zu? Noch offen.

#### Seite „Home-Tasten“

```
Display › Home-Tasten
──────────────────────────────────────────────────────────────
 ⓘ Aktion beim Halten einer Richtungstaste auf dem Home-Bildschirm.
   OK ist fest mit dem Prog-Modus belegt.

 Halten ▲                      [ Pause                       ▼ ]
 Halten ▼                      [ Reboot                      ▼ ]
 Halten ◀                      [ Display aus                 ▼ ]
 Halten ▶                      [ Screenshot                  ▼ ]
```

| Parameter | Auswahl | Vorgabe |
|---|---|---|
| Halten ▲ | –, Pause, Reboot, Prog-Modus, Display aus, Screenshot | Pause |
| Halten ▼ | wie oben | Reboot |
| Halten ◀ | wie oben | Display aus |
| Halten ▶ | wie oben | Screenshot |

**Hilfe „Home-Tasten“**

> Wird eine Richtungstaste auf dem Home-Bildschirm gehalten, läuft ein Countdown und danach die gewählte
> Aktion. Loslassen vor dem Ende bricht ab. Kurz gedrückt wechseln ◀ ▶ das Widget und ▲ ▼ die Seite. Im
> Menü dienen die Tasten nur zum Blättern.
>
> **Pause:** hält den Wechsel der Widgets an und setzt ihn wieder fort.
> **Reboot:** startet das Gerät neu.
> **Prog-Modus:** schaltet den KNX-Programmiermodus um.
> **Display aus:** schaltet das Display aus, bis wieder eine Taste gedrückt wird.
> **Screenshot:** speichert das aktuelle Bild auf der SD-Karte.
> **–:** die Taste hat beim Halten keine Aktion.
>
> Die OK-Taste schaltet immer den Prog-Modus um und lässt sich nicht umbelegen.

#### Seite „Experte“

```
Display › Experte
──────────────────────────────────────────────────────────────
 ⚠ Diese Werte passen die Ansteuerung des Displays an.
   Ein unpassender Wert kann das Bild unlesbar machen.

 Pre-Charge                    [ 5 – lang                    ▼ ]
 Refresh                       [ 3                           ▼ ]
 Screenshot invertieren        [ ]
```

| Parameter | Auswahl | Vorgabe |
|---|---|---|
| Pre-Charge | 0 … 5, kurz → lang | 5 |
| Refresh | 0 … 5, langsam → schnell | 3 |
| Screenshot invertieren | ja / nein | nein |

**Hilfe „Pre-Charge“ und „Refresh“**

> Feinabstimmung für das verbaute Display. Die Vorgaben passen für die üblichen Displays und müssen
> normalerweise nicht geändert werden.
>
> Ist das Bild nach einer Änderung nicht mehr lesbar, stellt diese Tastenfolge am Gerät alle
> Display-Einstellungen auf die Vorgaben zurück: ▲ ▲ ▼ ▼ ◀ ▶ ◀ ▶ OK OK.

**Hilfe „Screenshot invertieren“**

> Speichert Screenshots mit dunkler Schrift auf hellem Grund statt so, wie das Display sie zeigt.

#### Was am Entwurf noch offen ist

- Bei „Gesperrt“ wirkt die Rückstell-Tastenfolge trotzdem? Ich meine ja – das ist der Notausgang, wenn das
  Bild nicht mehr lesbar ist. Sie setzt dann auf die ETS-Werte zurück, nicht auf die Vorgaben der Firmware.
- Bei „Gesperrt“: Sind die Halte-Aktionen der Home-Tasten (Reboot, Prog-Modus) auch gesperrt, oder nur das
  Ändern der Einstellungen? Das eine ist „nichts verstellen“, das andere „nichts auslösen“ – das sollten
  zwei getrennte Schalter sein.
- „Time“ steht heute mit 4,5 s im Gerät; die ETS-Liste kennt 5 s als kleinsten Wert.
- Pre-Charge und Refresh werden am Gerät erst nach „Speichern“ übernommen, damit ein schlechter Wert einen
  Neustart nicht überlebt. Kommen sie aus der ETS, gibt es diese Sicherung nicht – dafür steht die
  Tastenfolge in der Hilfe.
- Anzahl der Plätze auf der Widget-Seite.
- Bildschirmschoner „Aus“: Im Code steht dazu „manager goes straight to SLEEP“. Prüfen, ob das Display
  dann schon zur Bildschirmschoner-Zeit ausgeht – wenn ja, muss das in die Hilfe.
- Alle Hilfetexte beim Einbau noch einmal Satz für Satz gegen den Code halten.

### Was es kostet

- **Neue ETS-Applikation.** Neue Parameter heißen neue knxprod und neue Minor-Version. Jeder muss einmal
  neu programmieren.
- **Flash.** Auf dem RP2040 mit 2 MB sind am Router noch rund 2,7 KB frei, am Interface rund 2,2 KB. Dafür
  muss vorher Platz her.
- **Unprogrammiertes Gerät.** Ohne ETS-Daten gibt es keine ETS-Werte, dann gelten die Standardwerte der
  Firmware – wie heute.
- **Noch nicht gemessen:** Nach einer neuen Applikationsversion ist das Gerät bis zum Download
  unprogrammiert. In der Zeit läuft das Display auf Standard und speichert nichts; die alten Werte bleiben
  im Flash und sollten danach wieder gelten. Das steht bisher nur im Code. Wer in dieser Zeit am Display
  etwas ändert, überschreibt damit womöglich die alten Werte, sobald das Gerät wieder programmiert ist.

### Reihenfolge

1. Schlüssel für die Widgets (`PRÄFIX.Schlüssel`), Entscheidung (a) oder (b) für die ETS.
2. ETS-Teil des Display-Moduls: die Einstellungen, „wer führt“, der Widget-Typ.
3. Firmware: ETS-Werte als Basis, lokaler Satz obendrauf, beim Download löschen wenn ETS führt, Sperre.
4. Einträge der anderen Module und der Produkte.
5. Der Knopf „Einstellungen vom Gerät holen“.

---

## 2 — Einstellungen im Web exportieren und importieren

Gibt es heute nicht. Das Gerät liefert aber schon alles als Text und nimmt jeden Wert einzeln an:

- `GET /display/config` – die Einstellungen mit Name und Wert.
- `GET /display/widgets` – die Widgets mit Reihenfolge, an/aus und Dauer.
- `POST /display/config?id=…&value=…` und `POST /display/widget?i=…&e|s|m=…` – je ein Wert.

Das lässt sich also allein in der Web-Seite bauen: Export speichert die beiden Antworten als Datei, Import
schickt die Werte der Reihe nach. Kein neuer Code in der Firmware, auf dem 2-MB-RP2040 kostet die Seite nur
ein paar hundert Byte.

- In der Datei stehen Namen und Werte, nicht der Flash-Block. Sie passt damit auch auf ein anderes Gerät
  und auf eine neuere Firmware; was nicht bekannt ist, wird übersprungen.
- Mühsam ist nur die Widget-Reihenfolge: Das Gerät kennt bloß „eins rauf“ und „eins runter“.
- Geht nur da, wo es die Web-Seite gibt.

---

## 3 — „Anzeige → Zurücksetzen“ startet das Gerät neu

Am RP2040-Router gemessen, mit der Firmware vor und nach der Widget-Speicherung: Nach „Ja“ im
Bestätigungsdialog ist das Gerät weg und kommt mit Uptime 0 wieder. Die Werte sind danach zurückgesetzt,
weil das Schreiben in den Flash noch vor dem Absturz passiert.

- Nicht die Ursache: das Flash-Schreiben aus dem Menü heraus. Mit verzögertem Schreiben (2 s später aus der
  Schleife) startet das Gerät genauso neu – und der Reset ist dann verloren. Der Versuch ist wieder raus.
- Übrig im Ablauf von `display_reset`: `_menuWidget->rebuild()` aus der eigenen Menü-Aktion heraus, während
  `handleConfirmButton()` und `selectItem()` noch laufen, und `showToast()`.
- Nächster Schritt: seriell mitschneiden, was beim Absturz kommt, dann `rebuild()` ansehen.
- Vermutlich bei KONAMI genauso (`triggerKonamiRestore()`, gleicher Aufbau). Nicht gemessen.
