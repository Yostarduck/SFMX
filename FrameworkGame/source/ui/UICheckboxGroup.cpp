#include "ui/UICheckboxGroup.h"
#include "ui/UICheckbox.h"

namespace sfmx
{

UICheckboxGroup::~UICheckboxGroup()
{
  clear();
}

void UICheckboxGroup::addCheckbox(UICheckbox* checkbox)
{
  if (!checkbox) return;
  checkbox->setGroup(this);
}

void UICheckboxGroup::removeCheckbox(UICheckbox* checkbox)
{
  if (!checkbox) return;

  for (size_t i = 0; i < m_checkboxes.size(); ++i)
  {
    if (m_checkboxes[i] == checkbox)
    {
      m_checkboxes.erase(m_checkboxes.begin() + static_cast<ptrdiff_t>(i));
      checkbox->m_group = nullptr;
      return;
    }
  }
}

void UICheckboxGroup::clear()
{
  for (auto* cb : m_checkboxes)
  {
    if (cb) cb->m_group = nullptr;
  }
  m_checkboxes.clear();
}

void UICheckboxGroup::setExclusive(bool exclusive)
{
  m_exclusive = exclusive;
}

bool UICheckboxGroup::isExclusive() const
{
  return m_exclusive;
}

UICheckbox* UICheckboxGroup::getChecked() const
{
  for (auto* cb : m_checkboxes)
  {
    if (cb && cb->isChecked()) return cb;
  }
  return nullptr;
}

void UICheckboxGroup::onCheckboxChecked(UICheckbox* checkbox)
{
  if (!m_exclusive) return;

  for (auto* cb : m_checkboxes)
  {
    if (cb && cb != checkbox && cb->isChecked())
    {
      cb->setChecked(false, true);
    }
  }
}

} // namespace sfmx
