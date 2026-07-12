/**
 * @file        DisplaySettingsStore.cpp
 * @brief       Implementation of the RAM-side display-settings persistence manager
 * @version     0.0.1
 * @date        2026-07-09
 * @copyright   Copyright (c) 2026, Erkan Çolak
 *              Licensed under GNU GPL v3.0
 *
 * Store behaviour + mock defaults. No flash access here.
 **/

#include "DisplaySettingsStore.h"

#ifdef DEVICE_DISPLAY_MODULE
    // requestSave/applyToRuntime touch the OpenKNX facades and live runtime objects (DD build only).
    #include "../Devices/i2cDisplay.h"
    #include "../WidgetsManager.h"
    #include "OpenKNX.h"
#endif

namespace
{
    // "Anzeigedauer" choices in deciseconds (100 ms units) as persisted in WidgetSetting.
    constexpr uint16_t DUR_5S = 50;
    constexpr uint16_t DUR_8S = 80;
    constexpr uint16_t DUR_10S = 100;
    constexpr uint16_t DUR_15S = 150;

    // --- Little-endian scalar read/write helpers (buffer, no flash) -----------

    inline void putU8(uint8_t *&p, uint8_t v)
    {
        *p++ = v;
    }

    inline void putU16(uint8_t *&p, uint16_t v)
    {
        *p++ = static_cast<uint8_t>(v & 0xFF);
        *p++ = static_cast<uint8_t>((v >> 8) & 0xFF);
    }

    inline void putU32(uint8_t *&p, uint32_t v)
    {
        *p++ = static_cast<uint8_t>(v & 0xFF);
        *p++ = static_cast<uint8_t>((v >> 8) & 0xFF);
        *p++ = static_cast<uint8_t>((v >> 16) & 0xFF);
        *p++ = static_cast<uint8_t>((v >> 24) & 0xFF);
    }

    inline uint8_t getU8(const uint8_t *&p)
    {
        return *p++;
    }

    inline uint16_t getU16(const uint8_t *&p)
    {
        uint16_t v = static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
        p += 2;
        return v;
    }

    inline uint32_t getU32(const uint8_t *&p)
    {
        uint32_t v = static_cast<uint32_t>(p[0]) |
                     (static_cast<uint32_t>(p[1]) << 8) |
                     (static_cast<uint32_t>(p[2]) << 16) |
                     (static_cast<uint32_t>(p[3]) << 24);
        p += 4;
        return v;
    }
} // namespace

void DisplaySettingsStore::loadDefaults()
{
    // DisplaySettings: values taken from the mock. "Anzeige" submenu:
    _settings.brightnessIdx = 3;         // ['25%','50%','75%','100%'] -> 100 %
    _settings.autoDim = true;            // Auto-Dimmen check
    _settings.invert = false;            // Invertieren check (fx:'inv')
    _settings.fontSizeIdx = 1;           // ['Normal','Groß','Größer'] -> Groß
    _settings.autoPaging = true;         // Seiten auto-blättern (fx:'autopage')
    _settings.screenSaverType = 0;       // idx 0..10 (Clock/Cube3D/Doom/.../Aus) -> 0 = Clock
    _settings.screenSaverTimeoutIdx = 2; // ['1 min','2 min','5 min','10 min'] -> 5 min
    _settings.sleepTimeoutIdx = 1;       // ['5 min','10 min','30 min','nie'] -> 10 min

    // "Home-Tasten" submenu: Oben=Pause, Unten=Reboot, Links=Display aus (#17), Rechts=—.
    _settings.keyMap[HOME_KEY_UP] = HomeKeyAction::Pause;
    _settings.keyMap[HOME_KEY_DOWN] = HomeKeyAction::Reboot;
    _settings.keyMap[HOME_KEY_LEFT] = HomeKeyAction::DisplayOff;
    _settings.keyMap[HOME_KEY_RIGHT] = HomeKeyAction::None;

    _settings.iconMenu = false; // root menu shows a text list by default (toggle in Anzeige)
    _settings.screenSaverCustomMin = 7;
    _settings.sleepCustomMin = 20;

    // Widget defaults from the mock "Widgets" submenu; array order = default rotation order.
    struct WidgetDefault
    {
        const char *name;
        bool enabled;
        uint16_t durationDs;
    };
    static const WidgetDefault DEFAULTS[] = {
        {"IP-Router", true, DUR_15S},
        {"System-Info", true, DUR_10S},
        {"KNX / BCU", true, DUR_8S},
        {"Time", false, DUR_10S},
        {"SD-Karte", true, DUR_8S},
    };

    _widgetCount = 0;
    for (const WidgetDefault &d : DEFAULTS)
    {
        if (_widgetCount >= WIDGET_SETTINGS_MAX) break;
        WidgetSetting &w = _widgets[_widgetCount];
        w.nameHash = hashWidgetName(d.name);
        w.orderIndex = _widgetCount;
        w.enabled = d.enabled;
        w.durationDs = d.durationDs;
        ++_widgetCount;
    }
    // Zero the unused tail so the fixed-size serialized blob is deterministic.
    for (size_t i = _widgetCount; i < WIDGET_SETTINGS_MAX; ++i)
        _widgets[i] = WidgetSetting{};

    // Defaults are a clean baseline; a factory reset marks dirty explicitly after.
    (void)DUR_5S; // reserved for future default use
    _dirty = false;
}

bool DisplaySettingsStore::upsertWidget(const WidgetSetting &ws)
{
    // Update in place if the widget is already known.
    for (uint8_t i = 0; i < _widgetCount; ++i)
    {
        if (_widgets[i].nameHash == ws.nameHash)
        {
            if (_widgets[i].orderIndex != ws.orderIndex ||
                _widgets[i].enabled != ws.enabled ||
                _widgets[i].durationDs != ws.durationDs)
            {
                _widgets[i].orderIndex = ws.orderIndex;
                _widgets[i].enabled = ws.enabled;
                _widgets[i].durationDs = ws.durationDs;
                _dirty = true;
            }
            return true;
        }
    }

    // Append a new record if there is room.
    if (_widgetCount >= WIDGET_SETTINGS_MAX) return false;
    _widgets[_widgetCount] = ws;
    ++_widgetCount;
    _dirty = true;
    return true;
}

void DisplaySettingsStore::clearWidgets()
{
    if (_widgetCount != 0) _dirty = true;
    for (size_t i = 0; i < WIDGET_SETTINGS_MAX; ++i)
        _widgets[i] = WidgetSetting{};
    _widgetCount = 0;
}

size_t DisplaySettingsStore::serialize(uint8_t *buf) const
{
    if (buf == nullptr) return 0;

    uint8_t *p = buf;

    // DisplaySettings scalars.
    putU8(p, _settings.brightnessIdx);
    putU8(p, _settings.autoDim ? 1 : 0);
    putU8(p, _settings.invert ? 1 : 0);
    putU8(p, _settings.fontSizeIdx);
    putU8(p, _settings.autoPaging ? 1 : 0);
    putU8(p, _settings.screenSaverType);
    putU8(p, _settings.screenSaverTimeoutIdx);
    putU8(p, _settings.sleepTimeoutIdx);
    for (size_t i = 0; i < HOME_KEY_COUNT; ++i)
        putU8(p, static_cast<uint8_t>(_settings.keyMap[i]));
    putU8(p, _settings.iconMenu ? 1 : 0);
    putU16(p, _settings.screenSaverCustomMin);
    putU16(p, _settings.sleepCustomMin);

    // Widget section: count then all WIDGET_SETTINGS_MAX records (fixed size).
    putU8(p, _widgetCount);
    for (size_t i = 0; i < WIDGET_SETTINGS_MAX; ++i)
    {
        const WidgetSetting &w = _widgets[i];
        putU32(p, w.nameHash);
        putU8(p, w.orderIndex);
        putU8(p, w.enabled ? 1 : 0);
        putU16(p, w.durationDs);
    }

    return static_cast<size_t>(p - buf); // == SERIALIZED_SIZE
}

bool DisplaySettingsStore::deserialize(const uint8_t *buf, size_t size)
{
    // Too short / null -> safe fallback to defaults.
    if (buf == nullptr || size < SERIALIZED_SIZE)
    {
        loadDefaults();
        return false;
    }

    const uint8_t *p = buf;

    _settings.brightnessIdx = getU8(p);
    _settings.autoDim = getU8(p) != 0;
    _settings.invert = getU8(p) != 0;
    _settings.fontSizeIdx = getU8(p);
    _settings.autoPaging = getU8(p) != 0;
    _settings.screenSaverType = getU8(p);
    _settings.screenSaverTimeoutIdx = getU8(p);
    _settings.sleepTimeoutIdx = getU8(p);
    for (size_t i = 0; i < HOME_KEY_COUNT; ++i)
        _settings.keyMap[i] = static_cast<HomeKeyAction>(getU8(p));
    _settings.iconMenu = getU8(p) != 0;
    _settings.screenSaverCustomMin = getU16(p);
    _settings.sleepCustomMin = getU16(p);

    uint8_t count = getU8(p);
    for (size_t i = 0; i < WIDGET_SETTINGS_MAX; ++i)
    {
        WidgetSetting &w = _widgets[i];
        w.nameHash = getU32(p);
        w.orderIndex = getU8(p);
        w.enabled = getU8(p) != 0;
        w.durationDs = getU16(p);
    }

    // Clamp the persisted count to capacity to stay consistent with the array.
    _widgetCount = (count > WIDGET_SETTINGS_MAX)
                       ? static_cast<uint8_t>(WIDGET_SETTINGS_MAX)
                       : count;

    // Freshly restored from storage -> clean baseline.
    _dirty = false;
    return true;
}

// --- Debounced, rate-limit-aware save request ---------------------------------

void DisplaySettingsStore::requestSave(bool force)
{
#ifdef DEVICE_DISPLAY_MODULE
    _dirty = true;
    _lastChangeMs = millis(); // restart the settle timer on every change

    // Do NOT flush flash synchronously here: requestSave() runs deep in the button/menu callback
    // chain and a flash write in that context reboots the RP2040. tickSave() commits from the loop.
    // force=true is only used from top-level contexts (factory reset / processBeforeRestart).
    if (force && knx.configured())
    {
        openknx.flash.save(true);
        _lastSaveRequestMs = millis();
        _lastChangeMs = 0; // nothing left pending
    }
#else
    // Non-DD build: keep dirty tracking coherent, no flash access available.
    (void)force;
    _dirty = true;
#endif
}

// Deferred, settled flash commit. Called every module loop at a shallow (non-callback) stack point;
// commits a pending change only after SETTLE_MS of no edits (one write per edit burst).
void DisplaySettingsStore::tickSave()
{
#ifdef DEVICE_DISPLAY_MODULE
    if (!_dirty || _lastChangeMs == 0 || !knx.configured()) return;

    const uint32_t now = millis();
    if ((now - _lastChangeMs) < SETTLE_MS) return; // still settling; wait for input to stop
    if (_lastSaveRequestMs != 0 && (now - _lastSaveRequestMs) < FLASH_SAVE_THROTTLE_MS) return;

    _lastSaveRequestMs = now;
    _lastChangeMs = 0;
    openknx.flash.save(); // shallow loop context - same place the KNX stack does its own saves
#endif
}

// --- Apply the store to the live runtime --------------------------------------

void DisplaySettingsStore::applyToRuntime(WidgetsManager *wm, i2cDisplay *disp)
{
#ifdef DEVICE_DISPLAY_MODULE
    // PowerSaveConfig (timeouts/brightness/auto-dim) from the persisted settings.
    if (wm != nullptr)
        wm->applyDisplaySettings(_settings);

    // Invert + font size go straight to the display hardware.
    if (disp != nullptr)
    {
        disp->setInvert(_settings.invert);
        disp->setFontSize(_settings.fontSizeIdx);
    }
#else
    (void)wm;
    (void)disp;
#endif
}
