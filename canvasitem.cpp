#include "canvasitem.h"

namespace {
constexpr int BoundingRectPadding = 6;
constexpr int StrokerWidthPadding = 10;
}

StrokeItem::StrokeItem(const QPainterPath &path, const QColor &color, int width)
    : m_path(path), m_color(color), m_width(width)
{
}

void StrokeItem::paint(QPainter &painter) const {
    QPen pen(m_color, m_width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(m_path);
}

QRectF StrokeItem::boundingRect() const {
    qreal padding = m_width + BoundingRectPadding;
    return m_path.boundingRect().adjusted(-padding, -padding, padding, padding);
}

bool StrokeItem::contains(const QPointF &scenePos) const {
    QPainterPathStroker stroker;
    stroker.setWidth(m_width + StrokerWidthPadding);
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);

    QPainterPath hitArea = stroker.createStroke(m_path);
    return hitArea.contains(scenePos);
}

void StrokeItem::moveBy(const QPointF &delta) {
    m_path.translate(delta);
}

std::unique_ptr<CanvasItem> StrokeItem::clone() const {
    return std::make_unique<StrokeItem>(QPainterPath(m_path), m_color, m_width);
}

void StrokeItem::transformFromRect(const QRectF &oldRect, const QRectF &newRect)
{
    if (oldRect.width() == 0.0 || oldRect.height() == 0.0) {
        return;
    }

    QTransform transform;

    transform.translate(newRect.left(), newRect.top());
    transform.scale(
        newRect.width() / oldRect.width(),
        newRect.height() / oldRect.height()
        );
    transform.translate(-oldRect.left(), -oldRect.top());

    m_path = transform.map(m_path);
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