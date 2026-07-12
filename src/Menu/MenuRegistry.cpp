#ifdef DEVICE_DISPLAY_MODULE

    #include "MenuRegistry.h"
    #include "OpenKNX.h"

    #include <algorithm>
    #include <climits>
    #include <utility>

namespace
{
    // About items carry no key, so match on element type.
    inline bool isAboutItem(const MenuConfig::MenuOption& item)
    {
        return item.type == MenuConfig::MenuElementType::About;
    }
} // namespace

const std::string& MenuRegistry::identityOf(const MenuOption& item)
{
    // Fall back to the label so key-less items (e.g. About) still dedupe.
    return item.key.empty() ? item.label : item.key;
}

int MenuRegistry::findRootIndex(const MenuOption& item) const
{
    const std::string& id = identityOf(item);
    if (id.empty()) return -1; // no identity -> always treat as distinct

    for (size_t i = 0; i < _rootItems.size(); ++i)
    {
        if (identityOf(_rootItems[i]) == id) return static_cast<int>(i);
    }
    return -1;
}

void MenuRegistry::registerRootItem(MenuOption item)
{
    const int existing = findRootIndex(item);
    if (existing >= 0)
    {
        // Replace in place to keep the original registration position and avoid duplicates.
        logDebugP("registerRootItem: replacing existing root item '%s'", identityOf(item).c_str());
        _rootItems[static_cast<size_t>(existing)] = std::move(item);
        _dirty = true;
        return;
    }

    logDebugP("registerRootItem: adding root item '%s'", identityOf(item).c_str());
    _rootItems.push_back(std::move(item));
    _dirty = true;
}

void MenuRegistry::registerRootItems(const std::vector<MenuOption>& items)
{
    _rootItems.reserve(_rootItems.size() + items.size());
    for (const auto& item : items)
    {
        registerRootItem(item); // by-value copy inside; keeps dedup logic in one place
    }
}

void MenuRegistry::registerRootItems(std::vector<MenuOption>&& items)
{
    _rootItems.reserve(_rootItems.size() + items.size());
    for (auto& item : items)
    {
        registerRootItem(std::move(item));
    }
    items.clear();
}

void MenuRegistry::registerAction(const std::string& key, ActionFn action)
{
    if (key.empty())
    {
        logErrorP("registerAction: ignoring action with empty key");
        return;
    }
    _actions[key] = std::move(action);
    _dirty = true; // callback merge happens on the next rebuild
}

void MenuRegistry::registerOnValueChanged(const std::string& key, OnValueChangedFn callback)
{
    if (key.empty())
    {
        logErrorP("registerOnValueChanged: ignoring callback with empty key");
        return;
    }
    _onValueChanged[key] = std::move(callback);
    _dirty = true;
}

void MenuRegistry::pinAboutLast()
{
    // Keep only the last-registered About entry (last wins) and move it to the
    // very end; if none exists, append a default so the root always trails with About.
    int lastAboutIdx = -1;
    for (size_t i = 0; i < _rootItems.size(); ++i)
    {
        if (isAboutItem(_rootItems[i])) lastAboutIdx = static_cast<int>(i);
    }

    if (lastAboutIdx < 0)
    {
        MenuOption about;
        about.label = "Ueber";
        about.type = MenuConfig::MenuElementType::About;
        _rootItems.push_back(std::move(about));
        _dirty = true;
        logDebugP("pinAboutLast: no About entry, appended default 'Über'");
        return;
    }

    MenuOption survivor = std::move(_rootItems[static_cast<size_t>(lastAboutIdx)]);
    _rootItems.erase(std::remove_if(_rootItems.begin(), _rootItems.end(), isAboutItem),
                     _rootItems.end());
    _rootItems.push_back(std::move(survivor));
    logDebugP("pinAboutLast: pinned single About entry last");
}

std::vector<MenuRegistry::MenuOption> MenuRegistry::build() const
{
    return buildWith({});
}

std::vector<MenuRegistry::MenuOption> MenuRegistry::buildWith(const std::vector<MenuOption>& transientRoots) const
{
    // Persistent roots first, then transient roots; a transient root that collides
    // with a persistent one is skipped so the module override wins. Returning a copy
    // keeps the transient tree unowned here, so it frees when the caller drops it.
    std::vector<MenuOption> result = _rootItems;
    result.reserve(result.size() + transientRoots.size());
    for (const auto& t : transientRoots)
    {
        if (findRootIndex(t) >= 0)
        {
            logDebugP("buildWith: skipping transient root '%s' (overridden by module root)",
                      identityOf(t).c_str());
            continue;
        }
        result.push_back(t);
    }

    // stable_sort keeps registration order among equal sortOrder; About is forced
    // to the max key so sorting never lifts it out of the tail.
    std::stable_sort(result.begin(), result.end(),
                     [](const MenuOption& a, const MenuOption& b) {
                         const int ka = isAboutItem(a) ? INT_MAX : a.sortOrder;
                         const int kb = isAboutItem(b) ? INT_MAX : b.sortOrder;
                         return ka < kb;
                     });

    // Ensure exactly one trailing About entry on the local copy so build() stays const.
    int lastAboutIdx = -1;
    for (size_t i = 0; i < result.size(); ++i)
    {
        if (isAboutItem(result[i])) lastAboutIdx = static_cast<int>(i);
    }
    if (lastAboutIdx < 0)
    {
        MenuOption about;
        about.label = "Ueber";
        about.type = MenuConfig::MenuElementType::About;
        result.push_back(std::move(about));
    }
    else
    {
        MenuOption survivor = std::move(result[static_cast<size_t>(lastAboutIdx)]);
        result.erase(std::remove_if(result.begin(), result.end(), isAboutItem), result.end());
        result.push_back(std::move(survivor));
    }

    logDebugP("build: %u root item(s) (About pinned last)", static_cast<unsigned>(result.size()));
    return result;
}

void MenuRegistry::clear()
{
    _rootItems.clear();
    _actions.clear();
    _onValueChanged.clear();
    _dirty = true; // force a rebuild after a reset
}

#endif // DEVICE_DISPLAY_MODULE
