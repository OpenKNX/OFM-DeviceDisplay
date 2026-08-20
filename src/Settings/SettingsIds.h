#pragma once
#ifdef DEVICE_DISPLAY_MODULE
/**
 * @file        SettingsIds.h
 * @brief       Single source of truth for the persisted display setting ids
 * @version     0.0.1
 * @date        2026-08-15
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 *
 * The ids, ranges and labels used by "ddc config set" and by the web settings page live here so the
 * two cannot drift apart. apply/read go through the DisplaySettingsStore; neither persists - the
 * caller decides when to save (console: right away, web: from loop()).
 **/
#include <cstddef>
#include <cstdint>

/**
 * How a setting wants to be edited. Toggle is 0/1, Number a plain range, Choice an index into a
 * named list (displayChoiceLabel()).
 */
enum class DisplaySettingKind : uint8_t
{
    Toggle = 0,
    Number = 1,
    Choice = 2
};

/**
 * One editable display setting: its id, how it is presented, and the range it is clamped to.
 */
struct DisplaySettingDef
{
    const char* id;
    const char* label; // UI label (German, matching the on-device menu)
    const char* hint;  // short range/unit hint, empty for toggles
    DisplaySettingKind kind;
    long min;
    long max;
    // group 0 = normal, 1 = expert (panel timing / screenshot detail), 2 = home-key hold action.
    // Only a presentation hint: every id is edited through the same apply/read path.
    uint8_t group;
};

enum : uint8_t
{
    DSG_NORMAL = 0,
    DSG_EXPERT = 1,
    DSG_HOMEKEY = 2
};

extern const DisplaySettingDef DISPLAY_SETTING_DEFS[];
extern const size_t DISPLAY_SETTING_DEF_COUNT;

/** @brief Look up a definition by id, or nullptr when the id is unknown. */
const DisplaySettingDef* findDisplaySetting(const char* id);

/** @brief Clamp value into the definition's range and write it to the settings store.
 *  @return false when the id is unknown (nothing is written). Does NOT persist or apply. */
bool applyDisplaySetting(const char* id, long value);

/** @brief Read the current value of a setting.
 *  @return false when the id is unknown (value untouched). */
bool readDisplaySetting(const char* id, long& value);

/** @brief Label for a Choice setting's option index, or nullptr when out of range. */
const char* displayChoiceLabel(const char* id, long index);

#endif // DEVICE_DISPLAY_MODULE
