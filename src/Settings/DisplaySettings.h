#pragma once
/**
 * @file        DisplaySettings.h
 * @brief       On-device display settings data model (POD, fixed size, no dynamic containers)
 * @version     0.0.1
 * @date        2026-07-09
 * @copyright   Copyright (c) 2026, Erkan Çolak
 *              Licensed under GNU GPL v3.0
 *
 * Persistable data model for the REG2 menu system. Index fields store the option index into the
 * mock choice list (doc/menu-system/reg2-menu-mockup.html), not a physical value. Header-only, no
 * dynamic memory/containers -> trivially copyable, fixed byte size for byte-buffer (de)serialization.
 **/

#include <cstddef>
#include <cstdint>

/**
 * Action bound to a Home-screen direction key ("Home-Tasten" submenu).
 * Enum value equals the mock option index.
 */
enum class HomeKeyAction : uint8_t
{
    None = 0,       // '—'          : no action
    Pause = 1,      // 'Pause'      : pause/resume home rotation
    Reboot = 2,     // 'Reboot'     : reboot the device
    Prog = 3,       // 'Prog-Mode'  : toggle KNX programming mode
    DisplayOff = 4, // 'Display aus': turn the display off (#17)
    Screenshot = 5  // 'Screenshot' : capture the framebuffer to SD. Values MUST match GestureAction.
};

/**
 * Index of a Home-screen direction key inside DisplaySettings::keyMap {UP, DOWN, LEFT, RIGHT}.
 */
enum HomeKeyIndex : uint8_t
{
    HOME_KEY_UP = 0,
    HOME_KEY_DOWN = 1,
    HOME_KEY_LEFT = 2,
    HOME_KEY_RIGHT = 3,
    HOME_KEY_COUNT = 4
};

/**
 * POD display settings model. Choice fields store the option index into the mock choice list
 * (ranges noted per field); defaults mirror the mock. Fixed size -> trivially copyable/serializable.
 */
struct DisplaySettings
{
    // "Helligkeit" idx 0..9 -> (idx+1)*10 = 10..100 %. Default 9 (100%). Granular 10% slider.
    uint8_t brightnessIdx = 9;

    // "Auto-Dim nach": minutes until the display dims; 0 = aus (no dim stage). Was a yes/no toggle.
    uint16_t dimMin = 15;

    // "Dim-Level" idx: 0 = nie (no dim), 1..9 -> 10..90 %. Effective dim = min(level, brightness).
    uint8_t dimLevelIdx = 3; // 30%

    // "Invertieren" check (fx:'inv') -> default false
    bool invert = false;

    // "Seiten auto-blättern" check (fx:'autopage') -> default true
    bool autoPaging = true;

    // "Bildschirmschoner" idx 0..10 (Clock/Cube3D/Doom/FireWorks/Life/Matrix/MatrixCl./Pong/Rain/Starfield/Aus)
    // -> default 0 = Clock. (Menu list in DefaultMenus.h must stay in this exact order = ScreenSaverType enum.)
    uint8_t screenSaverType = 0;

    // "Screensaver nach": minutes until the screensaver starts; 0 = aus. Edited via the number editor.
    uint16_t screenSaverMin = 30;

    // "Display aus nach": minutes until the display sleeps (off); 0 = nie. Edited via the number editor.
    uint16_t sleepMin = 60;

    // --- "Display" hardware tuning (Anzeige -> Display). Live-previewed, saved ONLY on manual confirm
    //     (so a bad value never auto-persists). "Zuruecksetzen" and the KONAMI code DO restore these to
    //     their safe defaults (below) - that is a recovery path, not a risk. ---
    bool displayRotate = false; // 180deg rotation (segment remap + COM scan) for upside-down mounting
    uint8_t preChargeIdx = 5;   // pre-charge preset idx (0..5); default = 0xF1 (bright, Adafruit init)
    uint8_t refreshIdx = 3;     // clock/refresh preset idx (0..5); default = 0x80 (Adafruit init)

    // "Home-Tasten" per-direction actions, indexed by HomeKeyIndex {UP,DOWN,LEFT,RIGHT}.
    // Defaults: UP=Pause, DOWN=Reboot, LEFT=Display off (#17), RIGHT=Screenshot.
    HomeKeyAction keyMap[HOME_KEY_COUNT] = {
        HomeKeyAction::Pause,      // UP
        HomeKeyAction::Reboot,     // DOWN
        HomeKeyAction::DisplayOff, // LEFT (#17: Left-hold -> display off)
        HomeKeyAction::Screenshot  // RIGHT (Right-hold -> screenshot to SD)
    };

    // "Icon-Menü": render the root menu as an icon grid instead of a text list -> default false.
    bool iconMenu = false;

    // "Screenshot" -> "Invertieren": false = lit pixels white (OLED look, default), true = paper look.
    bool screenshotInvert = false;
};

/**
 * Per-widget persisted settings. Widgets are identified by a name hash (see hashWidgetName) so the
 * record stays fixed-size. durationDs: display duration in deciseconds (1 ds = 100 ms).
 */
struct WidgetSetting
{
    uint32_t nameHash = 0;   // FNV-1a hash of the widget name (see hashWidgetName)
    uint8_t orderIndex = 0;  // Position in the rotation order ("Reihenfolge")
    bool enabled = true;     // "Anzeigen" check
    uint16_t durationDs = 0; // "Anzeigedauer" in deciseconds (100 ms units)
};

/**
 * Fixed capacity for the widget-settings array, with headroom above the registered widgets.
 */
static constexpr size_t WIDGET_SETTINGS_MAX = 16;

/**
 * Deterministic FNV-1a (32-bit) hash of a widget name, stable across reboots/builds/targets.
 * @param name null-terminated widget name (nullptr treated as empty string)
 * @return 32-bit FNV-1a hash
 */
constexpr uint32_t hashWidgetName(const char *name)
{
    uint32_t hash = 2166136261u; // FNV offset basis
    if (name != nullptr)
    {
        for (const char *p = name; *p != '\0'; ++p)
        {
            hash ^= static_cast<uint32_t>(static_cast<uint8_t>(*p));
            hash *= 16777619u; // FNV prime
        }
    }
    return hash;
}
