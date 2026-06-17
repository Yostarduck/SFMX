#include "ui/UIEventSystem.h"

#include "input/Keyboard.h"
#include "input/Mapping.h"
#include "input/ActionMap.h"
#include "input/InputAction.h"

namespace sfmx
{

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

void
UIEventSystem::onShutDown() {
}

// -----------------------------------------------------------------------------
// Init
// -----------------------------------------------------------------------------

void
UIEventSystem::init(Mapping* uiMapping) {
  ActionMap* map = uiMapping->findMap("UI");
  if (!map) return;

  if (InputAction* a = map->findAction("Submit"))
    m_submitSub = a->onStarted([this](const InputContext&) { m_submit = true; });

  if (InputAction* a = map->findAction("Cancel"))
    m_cancelSub = a->onStarted([this](const InputContext&) { m_cancel = true; });

  if (InputAction* a = map->findAction("Tab"))
    m_tabSub = a->onStarted([this](const InputContext&) {
      m_tab   = true;
      m_shift = Keyboard::isStarted() &&
                (Keyboard::instance().isPressed(Key::kLShift) ||
                 Keyboard::instance().isPressed(Key::kRShift));
    });
}

// -----------------------------------------------------------------------------
// Per-frame reset
// -----------------------------------------------------------------------------

void
UIEventSystem::beginFrame() {
  m_submit = false;
  m_cancel = false;
  m_tab    = false;
  m_shift  = false;
}

} // namespace sfmx
