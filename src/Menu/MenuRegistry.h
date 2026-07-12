#ifdef DEVICE_DISPLAY_MODULE
    #pragma once

    #include "MenuConfig.h"

    #include <functional>
    #include <string>
    #include <unordered_map>
    #include <vector>

// Central collection point for menu contributions. Modules (OAM/OFM) register
// their own root menu items plus the action / onValueChanged callbacks that
// back them; the DeviceDisplay module owns one MenuRegistry instance (a plain
// member, NOT a global singleton) and hands the assembled menu to the MenuWidget.
//
// Ownership / lifetime convention:
//   * MenuOptions are stored BY VALUE (std::move where possible). A caller may
//     let its local MenuOption go out of scope immediately after registering —
//     the registry keeps its own copy.
//   * Callbacks (action / onValueChanged) MUST only capture stable pointers or
//     the openknx facade (never references to caller-local menu options or
//     other short-lived stack objects), because they outlive the registration
//     call and are invoked later from the MenuWidget.
//
// Root items are deduplicated by identity key (MenuOption::key if non-empty,
// else MenuOption::label). Re-registering an item with the same identity
// REPLACES the previous entry in place (keeping its original position).
// registerAction / registerOnValueChanged overwrite by key (last wins).
class MenuRegistry
{
  public:
    using MenuOption = MenuConfig::MenuOption;
    using ActionFn = std::function<void()>;
    using OnValueChangedFn = std::function<void(const MenuConfig::MenuOption&, const MenuValue&)>;

    MenuRegistry() = default;
    ~MenuRegistry() = default;

    MenuRegistry(const MenuRegistry&) = delete;
    MenuRegistry& operator=(const MenuRegistry&) = delete;
    MenuRegistry(MenuRegistry&&) = default;
    MenuRegistry& operator=(MenuRegistry&&) = default;

    // Adds a single root menu item. If an item with the same identity key
    // already exists, it is replaced in place instead of added.
    void registerRootItem(MenuOption item);

    void registerRootItems(const std::vector<MenuOption>& items);
    void registerRootItems(std::vector<MenuOption>&& items);

    // key -> callback. Last registration for a key wins (overwrite).
    void registerAction(const std::string& key, ActionFn action);
    void registerOnValueChanged(const std::string& key, OnValueChangedFn callback);

    // Returns the assembled root menu: deduplicated, then stable-sorted by
    // MenuOption::sortOrder (equal sortOrder keeps registration order), with
    // exactly one "About" entry pinned last. A fresh vector is returned on every
    // call so the caller may move/own it freely; the registry keeps its own copy.
    std::vector<MenuOption> build() const;

    // Like build(), but merges caller-owned TRANSIENT roots (the heavy
    // display-owned tree) with the registry's own persistent module roots
    // WITHOUT storing the transient ones: the ~17 KiB display tree lives only in
    // the returned vector (freed by closeMenu), and the registry permanently
    // holds just the tiny module roots. A module root with the same identity as
    // a transient root wins (skips the transient), preserving "last wins".
    std::vector<MenuOption> buildWith(const std::vector<MenuOption>& transientRoots) const;

    // Guarantee the root ends with exactly one About entry, regardless of
    // registration order. Merges duplicate About items (last wins) and, if no
    // module contributed one, appends a default "Über" entry. Mutates the stored
    // root list in place; build() re-applies the pin defensively on its copy.
    void pinAboutLast();

    // Merged callback registries, for the MenuWidget to fold into its own.
    inline const std::unordered_map<std::string, ActionFn>& getActions() const { return _actions; }
    inline const std::unordered_map<std::string, OnValueChangedFn>& getOnValueChanged() const { return _onValueChanged; }

    inline size_t rootItemCount() const { return _rootItems.size(); }
    inline bool empty() const { return _rootItems.empty(); }
    void clear();

    // Dirty tracking for late module registrations. Any mutation flags the
    // registry dirty; the MenuWidget checks isDirty() before a root redraw and
    // rebuilds only while it is at the root level, then calls markClean(). Lets
    // a module registered after the startup delay appear without disrupting an
    // in-progress navigation.
    inline bool isDirty() const { return _dirty; }
    inline void markClean() { _dirty = false; }

    // Log prefix so the logInfoP / logDebugP / logErrorP macros resolve for
    // this (non-module) class.
    const std::string logPrefix() const { return "MenuRegistry"; }

  private:
    // Identity used for root-item deduplication: key if set, else label.
    static const std::string& identityOf(const MenuOption& item);

    // Returns index of an existing root item sharing item's identity, or -1.
    int findRootIndex(const MenuOption& item) const;

    std::vector<MenuOption> _rootItems;
    std::unordered_map<std::string, ActionFn> _actions;
    std::unordered_map<std::string, OnValueChangedFn> _onValueChanged;

    // Set on any mutation, cleared by the consumer after a rebuild.
    // Starts true so the very first build() is always performed.
    bool _dirty = true;
};

#endif // DEVICE_DISPLAY_MODULE
