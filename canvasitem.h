#ifndef CANVASITEM_H
#define CANVASITEM_H

#include <QImage>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QJsonObject>

#include <cstdint>
#include <QMap>
#include <QByteArray>
#include <memory>
#include <vector>

#include "hitbox.h"

using CanvasItemId = std::uint64_t;
constexpr CanvasItemId InvalidCanvasItemId = 0;

struct CanvasSerializationContext {
    QMap<QString, QByteArray> assets;
    int nextAssetIndex = 1;

    QString addImageAsset(const QImage &image);
};

struct CanvasDeserializationContext {
    QMap<QString, QByteArray> assets;

    QImage imageAsset(const QString &path) const;
};

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
    virtual QJsonObject serialize(CanvasSerializationContext &context) const = 0;
    virtual bool deserialize(const QJsonObject &json, const CanvasDeserializationContext &context) = 0;

    static std::unique_ptr<CanvasItem> deserializeItem(
        const QJsonObject &json,
        const CanvasDeserializationContext &context
        );

private:
    CanvasItemId m_id = InvalidCanvasItemId;
};

#endif // CANVASITEM_H
