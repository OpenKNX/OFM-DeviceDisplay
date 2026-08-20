#include "Settings/SettingsIds.h"
#ifdef DEVICE_DISPLAY_MODULE
    #include "DeviceDisplay.h"
    #include <cstring>

// Ranges are the ones the console has always clamped to; the labels mirror the on-device menu.
const DisplaySettingDef DISPLAY_SETTING_DEFS[] = {
    {"brightness", "Helligkeit", "0..9 = 10..100 %", DisplaySettingKind::Number, 0, 9, DSG_NORMAL},
    {"dim_after", "Auto-Dim nach", "Minuten, 0 = aus", DisplaySettingKind::Number, 0, 999, DSG_NORMAL},
    {"dim_level", "Dim-Level", "0 = nie, 1..9 = 10..90 %", DisplaySettingKind::Number, 0, 9, DSG_NORMAL},
    {"invert", "Invertieren", "", DisplaySettingKind::Toggle, 0, 1, DSG_NORMAL},
    {"auto_paging", "Seiten auto-blättern", "", DisplaySettingKind::Toggle, 0, 1, DSG_NORMAL},
    {"icon_menu", "Icon-Menü", "", DisplaySettingKind::Toggle, 0, 1, DSG_NORMAL},
    {"screensaver_type", "Bildschirmschoner", "", DisplaySettingKind::Choice, 0, 10, DSG_NORMAL},
    {"screensaver_after", "Screensaver nach", "Minuten, 0 = aus", DisplaySettingKind::Number, 0, 999, DSG_NORMAL},
    {"sleep_after", "Display aus nach", "Minuten, 0 = nie", DisplaySettingKind::Number, 0, 999, DSG_NORMAL},
    {"rotate", "Um 180° drehen", "", DisplaySettingKind::Toggle, 0, 1, DSG_NORMAL},
    // Panel timing + screenshot detail: rarely touched, and a bad value is only visible on the panel.
    {"precharge", "Pre-Charge", "0..5, kurz -> lang", DisplaySettingKind::Number, 0, 5, DSG_EXPERT},
    {"refresh", "Refresh", "0..5, langsam -> schnell", DisplaySettingKind::Number, 0, 5, DSG_EXPERT},
    {"screenshot_invert", "Screenshot invertieren", "", DisplaySettingKind::Toggle, 0, 1, DSG_EXPERT},
    // Home-screen hold actions (>= ~4 s on the respective key). Same store, shown next to the pad.
    {"homekey_up", "Halten ▲", "", DisplaySettingKind::Choice, 0, 5, DSG_HOMEKEY},
    {"homekey_down", "Halten ▼", "", DisplaySettingKind::Choice, 0, 5, DSG_HOMEKEY},
    {"homekey_left", "Halten ◀", "", DisplaySettingKind::Choice, 0, 5, DSG_HOMEKEY},
    {"homekey_right", "Halten ▶", "", DisplaySettingKind::Choice, 0, 5, DSG_HOMEKEY},
};

const size_t DISPLAY_SETTING_DEF_COUNT = sizeof(DISPLAY_SETTING_DEFS) / sizeof(DISPLAY_SETTING_DEFS[0]);

namespace
{
    // Order MUST match the ScreenSaverType enum and the DefaultMenus dropdown.
    const char* const kScreenSaverLabels[] = {
        "Clock", "Cube3D", "Doom", "FireWorks", "Life", "Matrix",
        "MatrixCl.", "Pong", "Rain", "Starfield", "Aus"};

    // Order MUST match the HomeKeyAction enum (it is the persisted value).
    const char* const kHomeKeyLabels[] = {
        "-", "Pause", "Reboot", "Prog-Modus", "Display aus", "Screenshot"};

    // id -> keyMap slot; nullptr-safe, returns HOME_KEY_COUNT when the id is not a home key.
    HomeKeyIndex homeKeyOf(const char* id)
    {
        if (strcmp(id, "homekey_up") == 0) return HOME_KEY_UP;
        if (strcmp(id, "homekey_down") == 0) return HOME_KEY_DOWN;
        if (strcmp(id, "homekey_left") == 0) return HOME_KEY_LEFT;
        if (strcmp(id, "homekey_right") == 0) return HOME_KEY_RIGHT;
        return HOME_KEY_COUNT;
    }

    long clampToDef(const DisplaySettingDef& def, long value)
    {
        if (value < def.min) return def.min;
        if (value > def.max) return def.max;
        return value;
    }
} // namespace

const DisplaySettingDef* findDisplaySetting(const char* id)
{
    if (id == nullptr) return nullptr;
    for (size_t i = 0; i < DISPLAY_SETTING_DEF_COUNT; ++i)
        if (strcmp(DISPLAY_SETTING_DEFS[i].id, id) == 0) return &DISPLAY_SETTING_DEFS[i];
    return nullptr;
}

bool applyDisplaySetting(const char* id, long value)
{
    const DisplaySettingDef* def = findDisplaySetting(id);
    if (def == nullptr) return false;

    const long v = clampToDef(*def, value);
    DisplaySettingsStore& store = openknxDisplayModule.getSettingsStore();

    if (strcmp(id, "brightness") == 0) store.setBrightnessIdx((uint8_t)v);
    else if (strcmp(id, "dim_after") == 0) store.setDimMin((uint16_t)v);
    else if (strcmp(id, "dim_level") == 0) store.setDimLevelIdx((uint8_t)v);
    else if (strcmp(id, "invert") == 0) store.setInvert(v != 0);
    else if (strcmp(id, "auto_paging") == 0) store.setAutoPaging(v != 0);
    else if (strcmp(id, "icon_menu") == 0) store.setIconMenu(v != 0);
    else if (strcmp(id, "screensaver_type") == 0) store.setScreenSaverType((uint8_t)v);
    else if (strcmp(id, "screensaver_after") == 0) store.setScreenSaverMin((uint16_t)v);
    else if (strcmp(id, "sleep_after") == 0) store.setSleepMin((uint16_t)v);
    else if (strcmp(id, "rotate") == 0) store.setDisplayRotate(v != 0);
    else if (strcmp(id, "precharge") == 0) store.setPreChargeIdx((uint8_t)v);
    else if (strcmp(id, "refresh") == 0) store.setRefreshIdx((uint8_t)v);
    else if (strcmp(id, "screenshot_invert") == 0) store.setScreenshotInvert(v != 0);
    else if (homeKeyOf(id) != HOME_KEY_COUNT) store.setKeyAction(homeKeyOf(id), (HomeKeyAction)v);
    else return false; // definition exists but no setter is wired -> refuse instead of silently ignoring

    return true;
}

bool readDisplaySetting(const char* id, long& value)
{
    if (findDisplaySetting(id) == nullptr) return false;
    const DisplaySettingsStore& store = openknxDisplayModule.settingsStore();

    if (strcmp(id, "brightness") == 0) value = store.brightnessIdx();
    else if (strcmp(id, "dim_after") == 0) value = store.dimMin();
    else if (strcmp(id, "dim_level") == 0) value = store.dimLevelIdx();
    else if (strcmp(id, "invert") == 0) value = store.invert() ? 1 : 0;
    else if (strcmp(id, "auto_paging") == 0) value = store.autoPaging() ? 1 : 0;
    else if (strcmp(id, "icon_menu") == 0) value = store.iconMenu() ? 1 : 0;
    else if (strcmp(id, "screensaver_type") == 0) value = store.screenSaverType();
    else if (strcmp(id, "screensaver_after") == 0) value = store.screenSaverMin();
    else if (strcmp(id, "sleep_after") == 0) value = store.sleepMin();
    else if (strcmp(id, "rotate") == 0) value = store.displayRotate() ? 1 : 0;
    else if (strcmp(id, "precharge") == 0) value = store.preChargeIdx();
    else if (strcmp(id, "refresh") == 0) value = store.refreshIdx();
    else if (strcmp(id, "screenshot_invert") == 0) value = store.screenshotInvert() ? 1 : 0;
    else if (homeKeyOf(id) != HOME_KEY_COUNT) value = (long)store.keyAction(homeKeyOf(id));
    else return false;

    return true;
}

const char* displayChoiceLabel(const char* id, long index)
{
    if (id == nullptr) return nullptr;
    if (strcmp(id, "screensaver_type") == 0)
    {
        if (index >= 0 && index < (long)(sizeof(kScreenSaverLabels) / sizeof(kScreenSaverLabels[0])))
            return kScreenSaverLabels[index];
    }
    else if (homeKeyOf(id) != HOME_KEY_COUNT)
    {
        if (index >= 0 && index < (long)(sizeof(kHomeKeyLabels) / sizeof(kHomeKeyLabels[0])))
            return kHomeKeyLabels[index];
    }
    return nullptr;
}

#endif // DEVICE_DISPLAY_MODULE
