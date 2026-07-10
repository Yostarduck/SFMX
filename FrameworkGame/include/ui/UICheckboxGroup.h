/************************************************************************/
/**
 * @file UICheckboxGroup.h
 * @author Swampertor
 * @date 2026/07/08
 * @brief  Manages a group of UICheckbox instances with optional radio-style exclusivity.
 */
/************************************************************************/
#pragma once

#include "core/platform/Prerequisites.h"

namespace sfmx
{

class UICheckbox;

/**
 * @brief  Collection of checkboxes that can enforce mutual exclusivity (radio behaviour).
 *
 * When @ref exclusive is true, checking one checkbox automatically unchecks all
 * other members of the group — standard radio-button semantics.
 */
class UICheckboxGroup
{
 public:
  UICheckboxGroup() = default;
  ~UICheckboxGroup();

  UICheckboxGroup(const UICheckboxGroup&) = delete;
  UICheckboxGroup& operator=(const UICheckboxGroup&) = delete;

  /** @brief  Add a checkbox to the group (also accessible via checkbox->setGroup). */
  void addCheckbox(UICheckbox* checkbox);
  /** @brief  Remove a checkbox from the group. */
  void removeCheckbox(UICheckbox* checkbox);
  /** @brief  Remove all checkboxes from the group. */
  void clear();

  /** @brief  Enable exclusive (radio) mode — only one checkbox may be checked. */
  void setExclusive(bool exclusive);
  /** @brief  Whether exclusive mode is active. */
  NODISCARD bool isExclusive() const;

  /** @brief  The first checked checkbox in the group, or nullptr. */
  NODISCARD UICheckbox* getChecked() const;

 private:
  friend class UICheckbox;

  void onCheckboxChecked(UICheckbox* checkbox);

  Vector<UICheckbox*> m_checkboxes;
  bool m_exclusive = false;
};

} // namespace sfmx
