#ifndef CANVASITEM_H
#define CANVASITEM_H

#include <QPainter>
#include <QPointF>
#include <QRectF>

#include <cstdint>
#include <memory>
#include <vector>

#include "hitbox.h"

using CanvasItemId = std::uint64_t;
constexpr CanvasItemId InvalidCanvasItemId = 0;

class CanvasItem : public HitboxOwner
{
public:
    virtual ~CanvasItem() = default;

    CanvasItemId id() const;
    void setId(CanvasItemId id);

    virtual void paint(QPainter &painter) const = 0;
    virtual QRectF boundingRect() const = 0;
    virtual bool contains(const QPointF &scenePos) const = 0;
    virtual void moveBy(const QPointF &delta) = 0;
    virtual void transformFromRect(const QRectF &oldRect, const QRectF &newRect) = 0;

    std::vector<Hitbox*> hitboxes() override;

    void onHitboxPressed(int role, const QPointF &scenePos) override;
    void onHitboxDragged(int role, const QPointF &scenePos) override;
    void onHitboxReleased(int role, const QPointF &scenePos) override;

    virtual std::unique_ptr<CanvasItem> clone() const = 0;

private:
    CanvasItemId m_id = InvalidCanvasItemId;
};

#endif // CANVASITEM_H
