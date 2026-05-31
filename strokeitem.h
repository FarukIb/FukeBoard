#ifndef STROKEITEM_H
#define STROKEITEM_H

#include "canvasitem.h"

#include <QColor>
#include <QPainterPath>
#include <Qt>

#include <memory>

class StrokeItem : public CanvasItem
{
public:
    StrokeItem() = default;
    StrokeItem(const QPainterPath &path, const QColor &color, int width);

    void paint(QPainter &painter) const override;
    QRectF boundingRect() const override;
    bool contains(const QPointF &scenePos) const override;
    void moveBy(const QPointF &delta) override;
    void transformFromRect(const QRectF &oldRect, const QRectF &newRect) override;

    std::unique_ptr<CanvasItem> clone() const override;

private:
    QPainterPath m_path;
    QColor m_color = Qt::black;
    int m_width = 3;
};

#endif // STROKEITEM_H
