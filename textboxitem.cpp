#include "textboxitem.h"

#include <QBrush>
#include <QJsonObject>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QSizeF>
#include <QTextDocument>

namespace {
const QSizeF DefaultTextBoxSize(260, 120);
constexpr qreal HandleSize = 20.0;
constexpr qreal MoveHandleHeight = 18.0;
constexpr qreal MinTextBoxWidth = 40.0;
constexpr qreal MinTextBoxHeight = 24.0;
const QColor TextBoxOutlineColour(0, 120, 215);
const QColor MoveHandleFillColour(0, 120, 215, 70);

QJsonObject rectToJson(const QRectF &rect)
{
    return QJsonObject {
        {"x", rect.x()},
        {"y", rect.y()},
        {"width", rect.width()},
        {"height", rect.height()}
    };
}

QRectF rectFromJson(const QJsonObject &json)
{
    return QRectF(
        json.value("x").toDouble(),
        json.value("y").toDouble(),
        json.value("width").toDouble(),
        json.value("height").toDouble()
        );
}
}

TextBoxItem::TextBoxItem()
{
    initializeHitboxes();
}

TextBoxItem::TextBoxItem(const TextBoxItem &other)
    : CanvasItem(other),
      m_rect(other.m_rect),
      m_html(other.m_html),
      m_viewScale(other.m_viewScale)
{
    initializeHitboxes();
}

TextBoxItem::TextBoxItem(const QPointF &position, const QString &html)
    : m_rect(position, DefaultTextBoxSize), m_html(html)
{
    initializeHitboxes();
}

TextBoxItem::TextBoxItem(const QRectF &rect, const QString &html)
    : m_rect(rect), m_html(html)
{
    initializeHitboxes();
}

const std::array<TextBoxItem::RoleData, TextBoxItem::Role::Count> &TextBoxItem::roleData()
{
    static const std::array<RoleData, Role::Count> data = {{
        RoleData {
            .resize = false,
            .cursorShape = Qt::SizeAllCursor
        },
        RoleData {
            .resize = true,
            .xFactor = 0.0,
            .yFactor = 0.0,
            .changesLeft = true,
            .changesTop = true,
            .cursorShape = Qt::SizeFDiagCursor
        },
        RoleData {
            .resize = true,
            .xFactor = 0.5,
            .yFactor = 0.0,
            .changesTop = true,
            .cursorShape = Qt::SizeVerCursor
        },
        RoleData {
            .resize = true,
            .xFactor = 1.0,
            .yFactor = 0.0,
            .changesTop = true,
            .changesRight = true,
            .cursorShape = Qt::SizeBDiagCursor
        },
        RoleData {
            .resize = true,
            .xFactor = 1.0,
            .yFactor = 0.5,
            .changesRight = true,
            .cursorShape = Qt::SizeHorCursor
        },
        RoleData {
            .resize = true,
            .xFactor = 1.0,
            .yFactor = 1.0,
            .changesRight = true,
            .changesBottom = true,
            .cursorShape = Qt::SizeFDiagCursor
        },
        RoleData {
            .resize = true,
            .xFactor = 0.5,
            .yFactor = 1.0,
            .changesBottom = true,
            .cursorShape = Qt::SizeVerCursor
        },
        RoleData {
            .resize = true,
            .xFactor = 0.0,
            .yFactor = 1.0,
            .changesLeft = true,
            .changesBottom = true,
            .cursorShape = Qt::SizeBDiagCursor
        },
        RoleData {
            .resize = true,
            .xFactor = 0.0,
            .yFactor = 0.5,
            .changesLeft = true,
            .cursorShape = Qt::SizeHorCursor
        }
    }};

    return data;
}

void TextBoxItem::paint(QPainter &painter) const
{
    QTextDocument document;
    document.setDefaultFont(textFont());
    document.setDefaultStyleSheet("body { color: black; }");
    document.setHtml(m_html);
    document.setTextWidth(m_rect.width());

    painter.save();
    painter.translate(m_rect.topLeft());
    document.drawContents(&painter, QRectF(QPointF(0, 0), m_rect.size()));
    painter.restore();

    paintChrome(painter);
}

QRectF TextBoxItem::boundingRect() const
{
    return m_rect.united(moveHandleRect()).adjusted(
        -HandleSize / 2.0,
        -HandleSize / 2.0,
        HandleSize / 2.0,
        HandleSize / 2.0
        );
}

bool TextBoxItem::contains(const QPointF &scenePos) const
{
    return m_rect.contains(scenePos) || moveHandleRect().contains(scenePos);
}

void TextBoxItem::moveBy(const QPointF &delta)
{
    m_rect.translate(delta);
    updateHitboxes();
}

void TextBoxItem::transformFromRect(const QRectF &oldRect, const QRectF &newRect)
{
    if (oldRect.width() == 0.0 || oldRect.height() == 0.0) {
        return;
    }

    qreal leftRatio = (m_rect.left() - oldRect.left()) / oldRect.width();
    qreal topRatio = (m_rect.top() - oldRect.top()) / oldRect.height();
    qreal rightRatio = (m_rect.right() - oldRect.left()) / oldRect.width();
    qreal bottomRatio = (m_rect.bottom() - oldRect.top()) / oldRect.height();

    qreal newLeft = newRect.left() + leftRatio * newRect.width();
    qreal newTop = newRect.top() + topRatio * newRect.height();
    qreal newRight = newRect.left() + rightRatio * newRect.width();
    qreal newBottom = newRect.top() + bottomRatio * newRect.height();

    m_rect = QRectF(
        QPointF(newLeft, newTop),
        QPointF(newRight, newBottom)
        ).normalized();
    updateHitboxes();
}

QRectF TextBoxItem::rect() const
{
    return m_rect;
}

void TextBoxItem::setRect(const QRectF &rect)
{
    m_rect = rect.normalized();
    updateHitboxes();
}

QString TextBoxItem::html() const
{
    return m_html;
}

void TextBoxItem::setHtml(const QString &html)
{
    m_html = html;
}

QString TextBoxItem::plainText() const
{
    QTextDocument document;
    document.setHtml(m_html);
    return document.toPlainText();
}

QFont TextBoxItem::textFont()
{
    QFont font;
    font.setPointSize(14);
    return font;
}

void TextBoxItem::setViewScale(qreal zoom)
{
    if (zoom <= 0.0) {
        m_viewScale = 1.0;
    } else {
        m_viewScale = zoom;
    }

    updateHitboxes();
}

std::unique_ptr<CanvasItem> TextBoxItem::clone() const
{
    return std::make_unique<TextBoxItem>(*this);
}

QJsonObject TextBoxItem::serialize(CanvasSerializationContext &context) const
{
    Q_UNUSED(context);

    return QJsonObject {
        {"type", "textBox"},
        {"id", QString::number(id())},
        {"rect", rectToJson(m_rect)},
        {"html", m_html}
    };
}

bool TextBoxItem::deserialize(const QJsonObject &json, const CanvasDeserializationContext &context)
{
    Q_UNUSED(context);

    m_rect = rectFromJson(json.value("rect").toObject()).normalized();
    m_html = json.value("html").toString();
    updateHitboxes();

    return true;
}

std::vector<Hitbox*> TextBoxItem::hitboxes()
{
    updateHitboxes();

    std::vector<Hitbox*> result;
    result.reserve(Role::Count);

    for (Hitbox &hitbox : m_hitboxes) {
        result.push_back(&hitbox);
    }

    return result;
}

void TextBoxItem::onHitboxPressed(int role, const QPointF &scenePos)
{
    if (!isValidRole(role)) {
        return;
    }

    m_lastDragScenePos = scenePos;
}

void TextBoxItem::onHitboxDragged(int role, const QPointF &scenePos)
{
    if (!isValidRole(role)) {
        return;
    }

    if (role == Role::MoveHandle) {
        moveBy(scenePos - m_lastDragScenePos);
        m_lastDragScenePos = scenePos;
        return;
    }

    if (isResizeRole(role)) {
        resizeFromRole(role, scenePos);
    }
}

void TextBoxItem::onHitboxReleased(int role, const QPointF &scenePos)
{
    Q_UNUSED(role);
    Q_UNUSED(scenePos);

    updateHitboxes();
}

bool TextBoxItem::isValidRole(int role) const
{
    return role >= 0 && role < Role::Count;
}

bool TextBoxItem::isResizeRole(int role) const
{
    return isValidRole(role) && roleData()[role].resize;
}

QPointF TextBoxItem::handleCenterForRole(Role role) const
{
    const RoleData &data = roleData()[role];

    return QPointF(
        m_rect.left() + data.xFactor * m_rect.width(),
        m_rect.top() + data.yFactor * m_rect.height()
        );
}

QRectF TextBoxItem::handleRectAt(const QPointF &center) const
{
    const qreal sceneSize = HandleSize / m_viewScale;

    return QRectF(
        center.x() - sceneSize / 2.0,
        center.y() - sceneSize / 2.0,
        sceneSize,
        sceneSize
        );
}

QRectF TextBoxItem::moveHandleRect() const
{
    return QRectF(
        m_rect.left(),
        m_rect.top() - MoveHandleHeight,
        m_rect.width(),
        MoveHandleHeight
        );
}

void TextBoxItem::initializeHitboxes()
{
    for (int role = 0; role < Role::Count; ++role) {
        m_hitboxes[role].owner = this;
        m_hitboxes[role].role = role;
        m_hitboxes[role].cursorShape = roleData()[role].cursorShape;
    }

    updateHitboxes();
}

void TextBoxItem::updateHitboxes()
{
    m_hitboxes[Role::MoveHandle].rect = moveHandleRect();

    for (int role = Role::TopLeft; role < Role::Count; ++role) {
        const QPointF center = handleCenterForRole(static_cast<Role>(role));
        m_hitboxes[role].rect = handleRectAt(center);
    }
}

void TextBoxItem::resizeFromRole(int role, const QPointF &scenePos)
{
    QRectF newRect = m_rect;
    const RoleData &data = roleData()[role];

    if (data.changesLeft) {
        newRect.setLeft(scenePos.x());
    }

    if (data.changesTop) {
        newRect.setTop(scenePos.y());
    }

    if (data.changesRight) {
        newRect.setRight(scenePos.x());
    }

    if (data.changesBottom) {
        newRect.setBottom(scenePos.y());
    }

    newRect = newRect.normalized();

    if (newRect.width() < MinTextBoxWidth) {
        newRect.setWidth(MinTextBoxWidth);
    }

    if (newRect.height() < MinTextBoxHeight) {
        newRect.setHeight(MinTextBoxHeight);
    }

    setRect(newRect);
}

void TextBoxItem::paintChrome(QPainter &painter) const
{
    painter.save();

    QPen outlinePen(TextBoxOutlineColour, 1, Qt::SolidLine);
    outlinePen.setCosmetic(true);
    painter.setPen(outlinePen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(m_rect);

    painter.setBrush(MoveHandleFillColour);
    painter.drawRect(moveHandleRect());

    painter.restore();
}
