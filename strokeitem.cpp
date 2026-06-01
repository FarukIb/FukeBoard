#include "strokeitem.h"

#include <QPainter>
#include <QPainterPathStroker>
#include <QJsonArray>
#include <QJsonObject>
#include <QPen>
#include <QTransform>

namespace {
constexpr int BoundingRectPadding = 6;
constexpr int StrokerWidthPadding = 10;
}

StrokeItem::StrokeItem(const QPainterPath &path, const QColor &color, int width)
    : m_path(path), m_color(color), m_width(width)
{
}

void StrokeItem::paint(QPainter &painter) const
{
    QPen pen(m_color, m_width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(m_path);
}

QRectF StrokeItem::boundingRect() const
{
    qreal padding = m_width + BoundingRectPadding;
    return m_path.boundingRect().adjusted(-padding, -padding, padding, padding);
}

bool StrokeItem::contains(const QPointF &scenePos) const
{
    QPainterPathStroker stroker;
    stroker.setWidth(m_width + StrokerWidthPadding);
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);

    QPainterPath hitArea = stroker.createStroke(m_path);
    return hitArea.contains(scenePos);
}

void StrokeItem::moveBy(const QPointF &delta)
{
    m_path.translate(delta);
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

std::unique_ptr<CanvasItem> StrokeItem::clone() const
{
    return std::make_unique<StrokeItem>(*this);
}

QJsonObject StrokeItem::serialize(CanvasSerializationContext &context) const
{
    Q_UNUSED(context);

    QJsonArray elements;
    for (int i = 0; i < m_path.elementCount(); ++i) {
        const QPainterPath::Element element = m_path.elementAt(i);
        QJsonObject elementJson;
        elementJson["type"] = element.type;
        elementJson["x"] = element.x;
        elementJson["y"] = element.y;
        elements.append(elementJson);
    }

    return QJsonObject {
        {"type", "stroke"},
        {"id", QString::number(id())},
        {"color", m_color.name(QColor::HexArgb)},
        {"width", m_width},
        {"path", elements}
    };
}

bool StrokeItem::deserialize(const QJsonObject &json, const CanvasDeserializationContext &context)
{
    Q_UNUSED(context);

    m_color = QColor(json.value("color").toString("#ff000000"));
    m_width = json.value("width").toInt(3);

    QPainterPath path;
    const QJsonArray elements = json.value("path").toArray();
    for (int i = 0; i < elements.size(); ++i) {
        const QJsonObject elementJson = elements.at(i).toObject();
        const auto type = static_cast<QPainterPath::ElementType>(elementJson.value("type").toInt());
        const qreal x = elementJson.value("x").toDouble();
        const qreal y = elementJson.value("y").toDouble();

        if (type == QPainterPath::MoveToElement) {
            path.moveTo(x, y);
        } else if (type == QPainterPath::LineToElement) {
            path.lineTo(x, y);
        } else if (type == QPainterPath::CurveToElement && i + 2 < elements.size()) {
            const QJsonObject controlTwoJson = elements.at(i + 1).toObject();
            const QJsonObject endJson = elements.at(i + 2).toObject();
            path.cubicTo(
                x,
                y,
                controlTwoJson.value("x").toDouble(),
                controlTwoJson.value("y").toDouble(),
                endJson.value("x").toDouble(),
                endJson.value("y").toDouble()
                );
            i += 2;
        }
    }

    m_path = path;
    return true;
}
