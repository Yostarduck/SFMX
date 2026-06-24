#include "ui/Canvas.h"

namespace sfmx
{

Canvas::Canvas(StringView name)
    : m_name(name)
{
}

Canvas::~Canvas()
{
    // Widgets are owned via unique_ptr — automatic destruction.
}

bool Canvas::destroyWidget(const UIWidget* widget)
{
    for (auto it = m_widgets.begin(); it != m_widgets.end(); ++it)
    {
        if (it->get() == widget)
        {
            m_widgets.erase(it);
            return true;
        }
    }
    return false;
}

void Canvas::clear()
{
    m_widgets.clear();
}

UIWidget* Canvas::hitTest(sf::Vector2f localPoint) const
{
    UIWidget* fallback = nullptr;

    // Iterate in reverse (back = drawn last = topmost).
    for (auto it = m_widgets.rbegin(); it != m_widgets.rend(); ++it)
    {
        UIWidget* w = it->get();
        if (!w->isEnabled() || !w->isVisible() || !w->isInteractable())
            continue;
        if (!w->containsPoint(localPoint))
            continue;
        if (w->isBlockingInput())
            return w;
        // Non-blocking widget: record and keep looking for a blocking one.
        if (fallback == nullptr)
            fallback = w;
    }
    return fallback;
}

void Canvas::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.transform *= m_transform;

    for (auto& w : m_widgets)
    {
        if (w->isVisible())
            w->onDraw(target, states);
    }
}

} // namespace sfmx
