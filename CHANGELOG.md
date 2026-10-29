# Changes

Display module for OpenKNX devices: OLED output, widgets, menu, gestures and a web view.
Entries are grouped by module version; every line traces to a commit in this repository.

## v0.1.1: 2026-10-29

Tagged as `v0.1.1`, which is the version `library.json` carries -- the tag and the file state the same
thing. The live view needs OFM-Network `ec/v0.9.0-beta.2`, which adds `trySendBinaryMessage()`.

**Web view**
* Feature: "Live (max. Rate)" streams the framebuffer as the binary message `dspFrame` over the UI WebSocket of OFM-Network. The device sends a frame only in a loop pass with free loop time and drops it when the send queue has no room; the page shows the frames per second and asks for the stream only while it is visible
* Change: the refresh choices are Aus, Alle 1 s (default), Alle 2 s, Alle 5 s and Live, before 4x/s, 2x/s (default), 1x/s and alle 5s. The invert checkbox is removed; the preview uses the stored invert setting
* Fix: while the panel is off the device sends no framebuffer: `/display/fb.bin` and `/display/shot.bmp` answer 204, the stream sends one frame without data, and the page clears the preview instead of showing the last image. The page tells a switched-off display apart from a switched-off transfer
* Change: the menu item has priority 40 (before 120) and follows WiFi (30)

**Display output**
* Fix: after a rotation the whole frame is sent again -- SEGREMAP applies only to data written after it, so unchanged columns stayed mirrored
* Change: on RP2040/RP2350 with `OPENKNX_I2C_SCHEDULER` the page chunks go to the I2C scheduler queue and `loop()` returns at once; a full queue leaves the rest of the page for the next pass, a failed chunk is sent again. Other platforms keep the synchronous push

**Widgets**
* Feature: `WidgetsManager::showDefaultWidget()` switches the rotation to a given widget; it acts only from the rotation view and returns whether the widget is shown, so a caller can retry
* Feature: `WidgetQRCode::drawQr()` draws a QR code with filled modules and a quiet zone; the WiFi widget of OFM-Network uses it too
* Fix: the QR widget drew its modules as outlines, hollow from 3 px up, and with a white background black modules on the black display -- both are drawn filled now
* Change: the QR code uses a fixed mask, 1.4 ms instead of 22 ms for a version 2 code on the RP2350; `OPENKNX_QR_AUTO_MASK` selects the scored mask. The buffers are sized for version 3 instead of 40

**Buttons**
* Change: with all five front buttons on one expander, a check reads the port once instead of five times

**Build**
* Change: the ddc console is built with `OPENKNX_CONSOLE_DEV`; `DDC_CONSOLE_DISABLE` remains as opt-out, and the console widget is built only together with the console
* Change: the root menu is ordered without `std::stable_sort` -- the same order, and the image shrinks by 14466 bytes

**Texts**
* Change: every text on the display is English -- menu, options, confirmations, toasts, the gesture and programming overlays and the clock. Dim after, Saver after and Sleep after fit one row with their value; the panel tuning submenu is Panel, the display settings submenu Display. The web page keeps its German labels

**Settings**
* Fix: the stored display settings take effect after a restart -- they were read from flash after the setup had applied the defaults, and nothing applied them afterwards
* Feature: visibility, duration and order of the rotation widgets are stored and restored; a widget without a stored record keeps the values it was registered with
* Change: a change is written to flash two seconds after the last edit instead of at most once in three minutes, and not at all when the state equals the stored one
* Change: the stored format is version 4; the settings of version 3 are kept, its widget records -- a fixed placeholder list -- are dropped
* Change: a setting added later is appended to the stored block and keeps the version, so an update keeps the settings

## v0.1.0: 2026-09-15

Tagged as `v0.1.0`, which is the version `library.json` carries -- the tag and the file state the same
thing.

**Widget manager**
* Feature: the manager owns the top-right corner and blinks a "BM" badge while a hardware busmonitor runs, so the state is visible on whatever widget is on screen
* Feature: a play triangle while rotation runs, two bars while it is paused
* Change: `drawPauseOverlay` is `drawManagerOverlay`, since it no longer shows only the pause state; widgets keep their page dots clear of `x >= w-12`

**Widgets**
* Fix: clock, console header and the system-info widget printed UTC where the user reads wall time -- all three use `getLocalTime()` now

**Web view**
* Feature: a web page shows the live OLED image, offers the joystick as buttons, and edits widgets and settings from the browser
* Feature: expanded settings with an expert group and a display-off overlay

**Build and robustness**
* Fix: the OTA overlay is rendered to completion during an RP2040 flash, where the loop is otherwise starved
* Fix: drop a duplicated `DDISP_HAS_NETWORK_MODULE` guard
* Change: `DDC_CONSOLE_DISABLE` also hides the "ddc" help line, which offered a command that was compiled out
* Change: add the ignore file the module was missing, so local build output stops showing up as untracked

## 0.1.0 - 2026-07-19

The release that turned the module from a widget carousel into an operable device front end:
persistent settings, a gesture engine, a menu with in-place editors, and an OTA overlay.

**Settings and menu**
* Feature: display settings are flash-backed and survive a restart
* Feature: menu system v2 -- a registry, value editors, and network IP configuration on the device itself
* Feature: in-place editors in menu v2, so a value is changed where it is shown
* Feature: power-save settings by the minute, plus control over the widget rotation
* Fix: the menu shows the persisted settings, a display switched off stays off, and the ESP loop task gets a larger stack

**Gestures**
* Feature: a 5-way gesture engine with an on-screen overlay, wired into the module core

**Widgets**
* Feature: About, Doom and WidgetTime, plus updates to the widget base class
* Feature: a full-screen overlay widget for OTA updates

**Diagnostics**
* Feature: the framebuffer can be written to SD as a 1-bit BMP screenshot
* Change: `DDCLoggerHelp` is replaced by a `ddc` console handler
* Change: non-blocking i2c diff-flush, so a display update no longer holds the loop

**Portability**
* Change: coupler guard, front buttons, CamelCase enums and portability fixes
* Change: module version 0.0.1 -> 0.1.0

## 0.0.1 - 2024-10-26 .. 2025-11-02

The build-up: display driver, widget system, manager, menu and the hardware breadth.

**Display driver**
* Feature: display geometry and I2C settings come from the build target instead of being hard-coded
* Feature: only the changed part of the framebuffer is sent to the display
* Feature: the transfer is split across loop passes (four columns each), to keep the loop time down
* Change: the driver uses `Wire` instead of a raw `i2c_inst_t`, and moved to `src/Devices`
* Feature: SSD1315 support next to SSD1306
* Feature: ESP32 target support next to RP2040, switchable by macro for multi-target builds

**Widgets and manager**
* Feature: dynamic text lines instead of fixed ones, with per-line alignment and optional empty lines
* Feature: QR-code widget with a fixed version and low ECC, because longer content is not readable on the display
* Feature: boot logo, prog-mode, clock, console, Cube3D, FireWorks and further widgets
* Feature: `WidgetFlags` replace the earlier `WidgetsAction`, with a background flag for widgets drawn behind others
* Feature: priority levels CRITICAL > HIGH > NORMAL > LOW decide which widget owns the screen
* Feature: several default widgets can share the rotation
* Feature: boot logo can be supplied externally
* Feature: a console widget scrolls log output onto the display
* Change: drawing of boot logo, prog mode and the default widget is split across loop passes
* Change: a background cache makes the manager 2.6 times faster
* Change: stopped and paused widgets are skipped in the loop
* Fix: prog mode was not shown while a partial transfer was running
* Fix: a widget with an invalid flag blocked the DEFAULT state
* Fix: Cube3D disappeared because its display pointer was reset

**Menu and input**
* Feature: menu widget, driven by the front-plate buttons and disabled when no front plate is detected
* Feature: support for the front-controller revision with a 5-button joystick
* Change: centralized button handling instead of per-widget reads
* Change: the obsolete GPIO-expander module is replaced by the common GPIO implementation

**Clock**
* Feature: analog and digital clock, the digital one carrying device information
* Feature: the clock shows the days since the last update once a threshold is reached

**Power and idle**
* Feature: the display dims after 60 s and resumes on the prog button
* Feature: power-save state flow with defined transitions back to ACTIVE
* Change: the idle timer has a 5 s minimum, because a default-widget rotation can take longer

**Structure and diagnostics**
* Change: `DeviceDisplay` is split into modular components; the old `Widgets` sources are gone
* Change: runtime statistics are collected for the widget class, to support loop-time work
* Feature: an `info` console command plus hardware validation
* Change: the module is excluded when the target device does not support a display

**Logo and QR fixes** (Cornelius Köpp)
* Fix: missing corners in the large OpenKNX logo, and its spacing to the bus symbol
* Fix: a missing corner in the small OpenKNX logo
* Fix: QR codes could not be scanned -- the colours were swapped
