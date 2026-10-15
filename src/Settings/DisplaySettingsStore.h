#pragma once
/**
 * @file        DisplaySettingsStore.h
 * @brief       RAM-side persistence manager for the on-device DisplaySettings model
 * @version     0.0.1
 * @date        2026-07-09
 * @copyright   Copyright (c) 2026, Erkan Çolak
 *              Licensed under GNU GPL v3.0
 *
 * Central RAM holder of the display-settings state for the REG2 menu system: owns one
 * DisplaySettings + a fixed WidgetSetting array, tracks dirty on mutation, and (de)serializes to a
 * byte buffer. Performs NO flash I/O; the DeviceDisplay module drives the flash lifecycle around it.
 **/

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "DisplaySettings.h"

// Forward-declared so the store stays header-light; the .cpp pulls in the full definitions.
class WidgetsManager;
class i2cDisplay;

/**
 * Holds the full RAM state of the display + per-widget settings, with dirty tracking and
 * byte-buffer (de)serialization. No flash access.
 */
class DisplaySettingsStore
{
  public:
    // Fixed byte size of the serialized blob (little-endian, hand-packed; see serialize()).
    // 9 u8 (brightness, invert, dimLevel, autoPaging, ssType, iconMenu, rotate, preCharge, refresh)
    // + 3 u16 (dimMin, screenSaverMin, sleepMin) + keyMap.
    static constexpr size_t SETTINGS_BYTES = 9 + 3 * 2 + HOME_KEY_COUNT;
    static constexpr size_t WIDGET_RECORD_BYTES = 4 + 1 + 1 + 2;     // hash + order + enabled + duration
    static constexpr size_t SERIALIZED_SIZE =
        SETTINGS_BYTES + 1 /*count*/ + (WIDGET_SETTINGS_MAX * WIDGET_RECORD_BYTES) + 1 /*screenshotInvert*/;

    DisplaySettingsStore()
    {
        loadDefaults();
    }

    // --- Lifecycle -----------------------------------------------------------

    /** Seed the settings with their defaults, drop all widget records and clear the dirty flag. */
    void loadDefaults();

    // --- Dirty tracking ------------------------------------------------------

    /** @return true if a setter changed state since the last markSaved()/loadDefaults(). */
    bool isDirty() const
    {
        return _dirty;
    }

    /** Force the dirty flag (e.g. factory reset wanting an immediate persist). */
    void markDirty()
    {
        _dirty = true;
    }

    /** Take the current state as the stored one (after a flash write or a restore) and clear dirty. */
    void markSaved();

    /** @return true if the current state differs from the one last written to or read from flash. */
    bool differsFromSaved() const;

    // --- Deferred save request ------------------------------------------------

    /**
     * Request that the current state be persisted. Only marks dirty (no synchronous write) unless
     * force==true; a forced save writes at once and is only safe from a top-level context.
     * No-op while !knx.configured().
     * @param force true to write at once instead of after the settle window.
     */
    void requestSave(bool force = false);

    /**
     * Deferred flash commit. Call every module loop (shallow stack); writes SETTLE_MS after the last
     * change, and only if the state differs from the stored one. Must NOT be called from a
     * button/menu callback (reboots RP2040).
     */
    void tickSave();

    static constexpr uint32_t SETTLE_MS = 2000; // settle window before a deferred save

    // --- Apply the store to the live runtime ----------------------------------

    /**
     * Push the persisted settings onto the running system: PowerSaveConfig via
     * WidgetsManager::applyDisplaySettings(), invert onto the i2cDisplay.
     * Safe with nullptr arguments (each handled independently).
     * @param wm   the live WidgetsManager (may be nullptr -> power config skipped).
     * @param disp the live i2cDisplay (may be nullptr -> invert skipped).
     */
    void applyToRuntime(WidgetsManager *wm, i2cDisplay *disp = nullptr);

    // --- DisplaySettings access ----------------------------------------------

    /** Read-only access to the whole settings struct. */
    const DisplaySettings &settings() const
    {
        return _settings;
    }

    uint8_t brightnessIdx() const { return _settings.brightnessIdx; }
    uint16_t dimMin() const { return _settings.dimMin; }
    uint8_t dimLevelIdx() const { return _settings.dimLevelIdx; }
    bool invert() const { return _settings.invert; }
    bool autoPaging() const { return _settings.autoPaging; }
    uint8_t screenSaverType() const { return _settings.screenSaverType; }
    uint16_t screenSaverMin() const { return _settings.screenSaverMin; }
    uint16_t sleepMin() const { return _settings.sleepMin; }
    bool iconMenu() const { return _settings.iconMenu; }
    bool screenshotInvert() const { return _settings.screenshotInvert; }
    bool displayRotate() const { return _settings.displayRotate; }
    uint8_t preChargeIdx() const { return _settings.preChargeIdx; }
    uint8_t refreshIdx() const { return _settings.refreshIdx; }

    /** @return action bound to a Home-screen key; None for out-of-range index. */
    HomeKeyAction keyAction(HomeKeyIndex key) const
    {
        if (key >= HOME_KEY_COUNT) return HomeKeyAction::None;
        return _settings.keyMap[key];
    }

    void setBrightnessIdx(uint8_t idx) { assign(_settings.brightnessIdx, idx); }
    void setDimMin(uint16_t v) { assign(_settings.dimMin, v); }
    void setDimLevelIdx(uint8_t idx) { assign(_settings.dimLevelIdx, idx); }
    void setInvert(bool v) { assign(_settings.invert, v); }
    void setAutoPaging(bool v) { assign(_settings.autoPaging, v); }
    void setScreenSaverType(uint8_t idx) { assign(_settings.screenSaverType, idx); }
    void setScreenSaverMin(uint16_t v) { assign(_settings.screenSaverMin, v); }
    void setSleepMin(uint16_t v) { assign(_settings.sleepMin, v); }
    void setIconMenu(bool v) { assign(_settings.iconMenu, v); }
    void setScreenshotInvert(bool v) { assign(_settings.screenshotInvert, v); }
    // Display hardware tuning (persisted only via the manual "Save" action).
    void setDisplayRotate(bool v) { assign(_settings.displayRotate, v); }
    void setPreChargeIdx(uint8_t idx) { assign(_settings.preChargeIdx, idx); }
    void setRefreshIdx(uint8_t idx) { assign(_settings.refreshIdx, idx); }

    /** Bind an action to a Home-screen key; no-op (and no dirty) for a bad index. */
    void setKeyAction(HomeKeyIndex key, HomeKeyAction action)
    {
        if (key >= HOME_KEY_COUNT) return;
        assign(_settings.keyMap[key], action);
    }

    // --- Widget settings access ----------------------------------------------

    /** @return number of live widget-settings records (0..WIDGET_SETTINGS_MAX). */
    uint8_t widgetCount() const
    {
        return _widgetCount;
    }

    static constexpr size_t widgetCapacity()
    {
        return WIDGET_SETTINGS_MAX;
    }

    /** Read-only access to a widget record by array index; nullptr if out of range. */
    const WidgetSetting *widgetAt(size_t index) const
    {
        if (index >= _widgetCount) return nullptr;
        return &_widgets[index];
    }

    /** Find a widget record by its name hash; nullptr if not present. */
    const WidgetSetting *findWidget(uint32_t nameHash) const
    {
        for (const WidgetSetting *w = _widgets, *end = _widgets + _widgetCount; w != end; ++w)
            if (w->nameHash == nameHash) return w;
        return nullptr;
    }

    /**
     * Insert or update a widget record identified by nameHash. Marks dirty when anything changed.
     * @return true if stored (updated or appended), false if the array is full.
     */
    bool upsertWidget(const WidgetSetting &ws);

    /** Remove all widget records. Marks dirty if any existed. */
    void clearWidgets();

    /** Replace all widget records by @p list (at most WIDGET_SETTINGS_MAX are taken). Marks dirty. */
    void replaceWidgets(const WidgetSetting *list, size_t count);

    // --- Serialization (byte buffer, no flash) --------------------------------

    /**
     * Write the full RAM state into @p buf as a fixed-size SERIALIZED_SIZE blob.
     * @param buf destination buffer (>= SERIALIZED_SIZE bytes); ignored if nullptr
     * @return number of bytes written (SERIALIZED_SIZE), or 0 if buf is nullptr
     */
    size_t serialize(uint8_t *buf) const;

    /**
     * Restore the RAM state from a serialize() blob; falls back to loadDefaults() on a short buffer.
     * A longer blob comes from a newer firmware: the known part is read, the rest ignored. A setting
     * added later goes to the end of the blob and is read only if the blob holds it.
     * Clears the dirty flag on success (fresh-from-storage baseline).
     * @param buf  source buffer
     * @param size number of valid bytes in @p buf
     * @return true on a full, consistent restore; false if defaults were applied
     */
    bool deserialize(const uint8_t *buf, size_t size);

  private:
    /** Assign + dirty helper: only marks dirty when the value actually changes. */
    template <typename T>
    void assign(T &dst, T value)
    {
        if (dst != value)
        {
            dst = value;
            _dirty = true;
        }
    }

    DisplaySettings _settings;                        // full scalar settings state
    WidgetSetting _widgets[WIDGET_SETTINGS_MAX] = {}; // per-widget records
    uint8_t _widgetCount = 0;                         // number of live records
    bool _dirty = false;                              // set by mutating setters

    uint8_t _saved[SERIALIZED_SIZE];      // state as last written to or read from flash
    bool _hasSaved = false;               // false until a first write or restore

    uint32_t _lastChangeMs = 0; // millis() of last change (0 = none pending); settle timer for tickSave()
};
