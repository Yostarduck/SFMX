#pragma once

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"
#include "utils/EventSystem.h"

namespace sfmx {

class SceneNode;

// ── Anchor helpers ──────────────────────────────────────────────────────────

struct Anchors {
  float left   = 0.f;
  float top    = 0.f;
  float right  = 1.f;
  float bottom = 1.f;

  static Anchors fill()           { return {0.f, 0.f, 1.f, 1.f}; }
  static Anchors center()         { return {0.5f, 0.5f, 0.5f, 0.5f}; }
  static Anchors topLeft()        { return {0.f, 0.f, 0.f, 0.f}; }
  static Anchors topRight()       { return {1.f, 0.f, 1.f, 0.f}; }
  static Anchors bottomLeft()     { return {0.f, 1.f, 0.f, 1.f}; }
  static Anchors bottomRight()    { return {1.f, 1.f, 1.f, 1.f}; }
  static Anchors stretchH(float l, float r) { return {l, 0.f, r, 1.f}; }
  static Anchors stretchV(float t, float b) { return {0.f, t, 1.f, b}; }
};

// ── UIWidget base class (mixin for widget components) ──────────────────────

class UIWidget {
 public:
  explicit UIWidget(SceneNode* owner);

  // ── Size ────────────────────────────────────────────────────────────────
  void         setSize(sf::Vector2f s) { m_size = s; }
  sf::Vector2f getSize() const         { return m_size; }

  // ── Anchors / pivot ────────────────────────────────────────────────────
  void           setAnchors(const Anchors& a) { m_anchors = a; }
  const Anchors& getAnchors() const           { return m_anchors; }
  void           setPivot(sf::Vector2f p)     { m_pivot = p; }
  sf::Vector2f   getPivot() const             { return m_pivot; }

  // ── Interactive state ──────────────────────────────────────────────────
  void setInteractable(bool v) { m_interactable = v; }
  bool isInteractable() const  { return m_interactable; }
  bool isHovered() const       { return m_hovered; }
  bool isPressed() const       { return m_pressed; }

  void setHovered(bool v);   // fires onHoverEnter / onHoverExit
  void setPressed(bool v);   // fires onPress / onRelease, and onClick on release

  // ── Hit-testing ────────────────────────────────────────────────────────
  NODISCARD sf::FloatRect getWorldRect() const;

  // ── Layout ─────────────────────────────────────────────────────────────
  void resolveLayout(sf::FloatRect parentRect);

  // ── Events ─────────────────────────────────────────────────────────────
  Event<void()>& onClick()       { return m_onClick; }
  Event<void()>& onHoverEnter()  { return m_onHoverEnter; }
  Event<void()>& onHoverExit()   { return m_onHoverExit; }
  Event<void()>& onPress()       { return m_onPress; }
  Event<void()>& onRelease()     { return m_onRelease; }

 protected:
  SceneNode*   m_owner;
  sf::Vector2f m_size        = {100.f, 30.f};
  Anchors      m_anchors;
  sf::Vector2f m_pivot       = {0.f, 0.f};
  bool         m_interactable = true;
  bool         m_hovered      = false;
  bool         m_pressed      = false;

  Event<void()> m_onClick;
  Event<void()> m_onHoverEnter;
  Event<void()> m_onHoverExit;
  Event<void()> m_onPress;
  Event<void()> m_onRelease;
};

} // namespace sfmx
