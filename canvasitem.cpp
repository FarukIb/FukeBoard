#include "canvasitem.h"

#include "imageitem.h"
#include "milegriditem.h"
#include "strokeitem.h"
#include "textboxitem.h"

#include <QBuffer>
#include <QIODevice>
#include <QJsonValue>

QString CanvasSerializationContext::addImageAsset(const QImage &image)
{
    const QString path = QString("assets/image_%1.png").arg(nextAssetIndex++);

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");

    assets.insert(path, bytes);
    return path;
}

QImage CanvasDeserializationContext::imageAsset(const QString &path) const
{
    QImage image;
    image.loadFromData(assets.value(path));
    return image;
}

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

std::unique_ptr<CanvasItem> CanvasItem::deserializeItem(
    const QJsonObject &json,
    const CanvasDeserializationContext &context
    )
{
    const QString type = json.value("type").toString();

    std::unique_ptr<CanvasItem> item;
    if (type == "stroke") {
        item = std::make_unique<StrokeItem>();
    } else if (type == "image") {
        item = std::make_unique<ImageItem>();
    } else if (type == "textBox") {
        item = std::make_unique<TextBoxItem>();
    } else if (type == "mileGrid") {
        item = std::make_unique<MileGridItem>();
    }

    if (!item || !item->deserialize(json, context)) {
        return nullptr;
    }

    item->setId(json.value("id").toString().toULongLong());
    return item;
}
