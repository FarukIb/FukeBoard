#include "canvasitem.h"

CanvasItemId CanvasItem::id() const
{
    return m_id;
}

void CanvasItem::setId(CanvasItemId id)
{
    m_id = id;
}

std::vector<Hitbox*> CanvasItem::hitboxes()
{
    return {};
}

void CanvasItem::onHitboxPressed(int role, const QPointF &scenePos)
{
    Q_UNUSED(role);
    Q_UNUSED(scenePos);
}

void CanvasItem::onHitboxDragged(int role, const QPointF &scenePos)
{
    Q_UNUSED(role);
    Q_UNUSED(scenePos);
}

void CanvasItem::onHitboxReleased(int role, const QPointF &scenePos)
{
    Q_UNUSED(role);
    Q_UNUSED(scenePos);
}
