#include "scripts/RegisterUIManager.h"

#include "ui/UIManager.h"
#include "ui/UIWidget.h"
#include "ui/UIWidgetRegistry.h"
#include "ui/UIButton.h"
#include "ui/UICheckbox.h"
#include "ui/UIHorizontalBox.h"
#include "ui/UIImage.h"
#include "ui/UILabel.h"
#include "ui/UIScrollView.h"
#include "ui/UISlider.h"
#include "ui/UITextBox.h"
#include "ui/UIVerticalBox.h"

#include "core/platform/Prerequisites.h"

namespace sfmx
{

namespace script
{

namespace
{

/**
 * @brief Push @p widget as its CONCRETE usertype — sol2 pushes the static
 *        type, so the widget type enum drives an explicit downcast (a bare
 *        UIWidget* would hide UILabel::setText & co from Lua). nullptr → nil.
 *        Shared by UI:get and UI:createRoot.
 */
sol::object
widgetObject(sol::state_view view, UIWidget* widget) {
  if (nullptr == widget) {
    return sol::make_object(view, sol::lua_nil);
  }
  switch (widget->getType()) {
    case WidgetType::kButton:        return sol::make_object(view, static_cast<UIButton*>(widget));
    case WidgetType::kLabel:         return sol::make_object(view, static_cast<UILabel*>(widget));
    case WidgetType::kImage:         return sol::make_object(view, static_cast<UIImage*>(widget));
    case WidgetType::kCheckbox:      return sol::make_object(view, static_cast<UICheckbox*>(widget));
    case WidgetType::kTextBox:       return sol::make_object(view, static_cast<UITextBox*>(widget));
    case WidgetType::kSlider:        return sol::make_object(view, static_cast<UISlider*>(widget));
    case WidgetType::kVerticalBox:   return sol::make_object(view, static_cast<UIVerticalBox*>(widget));
    case WidgetType::kHorizontalBox: return sol::make_object(view, static_cast<UIHorizontalBox*>(widget));
    case WidgetType::kScrollView:    return sol::make_object(view, static_cast<UIScrollView*>(widget));
    case WidgetType::kUnknown:       break;
  }
  return sol::make_object(view, widget);
}

/**
 * @brief Script-facing kind name → type UUID for UI:createRoot (keys the
 *        Phase 5 factory registry).
 * @return nullptr when @p type is not a known widget kind.
 */
const UUID*
widgetTypeIdForName(const String& type) {
  static const UnorderedMap<String, UUID> kWidgetTypes = {
    {"button",        TypeTraits<UIButton>::getTypeId()},
    {"label",         TypeTraits<UILabel>::getTypeId()},
    {"image",         TypeTraits<UIImage>::getTypeId()},
    {"checkbox",      TypeTraits<UICheckbox>::getTypeId()},
    {"textbox",       TypeTraits<UITextBox>::getTypeId()},
    {"slider",        TypeTraits<UISlider>::getTypeId()},
    {"verticalbox",   TypeTraits<UIVerticalBox>::getTypeId()},
    {"horizontalbox", TypeTraits<UIHorizontalBox>::getTypeId()},
    {"scrollview",    TypeTraits<UIScrollView>::getTypeId()},
  };
  auto it = kWidgetTypes.find(type);
  return (it != kWidgetTypes.end()) ? &it->second : nullptr;
}

} // namespace

void
registerUIManager(sol::state_view lua) {
  lua.new_usertype<UIManager>("UIManager",
    sol::no_constructor,

    // UI:get(name) — recursive lookup over all widget roots; nil when absent.
    // Concrete-type downcast lives in widgetObject() (shared with createRoot).
    "get", [](sol::this_state state, UIManager& manager, const String& name)
      -> sol::object {
      return widgetObject(sol::state_view(state), manager.findByName(name));
    },

    // UI:createRoot(type[, name]) — build a widget of the given kind through
    // the Phase 5 factory registry (authored default size), root it in the
    // manager (its slot applies on add), and return it as its concrete
    // usertype. type ∈ button | label | image | checkbox | textbox | slider |
    // verticalbox | horizontalbox | scrollview. Unknown kind, missing
    // registry, or exhausted pool → nil. Roots append; use UI:get to reach
    // pre-existing ones.
    "createRoot", [](sol::this_state state, UIManager& manager,
                     const String& type, sol::optional<String> name)
      -> sol::object {
      sol::state_view view(state);
      const UUID* typeId = widgetTypeIdForName(type);
      if (nullptr == typeId || !UIWidgetRegistry::isStarted()) {
        return sol::make_object(view, sol::lua_nil);
      }
      UIWidget* widget =
          UIWidgetRegistry::instance().create(*typeId, sf::Vector2f{});
      if (nullptr == widget) {
        return sol::make_object(view, sol::lua_nil);
      }
      if (name.has_value() && !name->empty()) {
        widget->setName(*name);
      }
      manager.addRoot(widget);
      return widgetObject(view, widget);
    },

    // UI:save(path) / UI:load(path) — round-trip the whole root set as a UI
    // document (.sfmxasset). load() APPENDS the loaded roots; it does not
    // clear existing widgets.
    "save", [](UIManager& manager, const String& path) -> bool {
      return manager.saveUI(path);
    },
    "load", [](UIManager& manager, const String& path) -> bool {
      return manager.loadUI(path);
    }
  );

  // Only bind the singleton global if the module is up: bindings are registered
  // from ScriptEngine::onStartUp, which may run before UIManager::startUp (e.g.
  // in tests that never bring the UI layer up). instance() throws if not started.
  if (UIManager::isStarted()) {
    lua["UI"] = std::ref(UIManager::instance());
  }
}

}  // namespace script

}  // namespace sfmx
