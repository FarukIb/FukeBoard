#include "milegriditem.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QPainter>
#include <QPen>
#include <algorithm>

namespace MileGridConstants {
constexpr int DefaultColumns = 16;
constexpr int DefaultRows = 8;
constexpr qreal DefaultCellSize = 24.0;
constexpr qreal HandleSize = 20.0;
constexpr qreal MoveHandleHeight = 18.0;
constexpr qreal MinGridWidth = 24.0;
constexpr qreal MinGridHeight = 24.0;

const QColor OutlineColour(0, 120, 215);
const QColor MoveHandleFillColour(0, 120, 215, 70);
const QColor GridLineColour(185, 185, 185);
const QColor DefaultCellColour(20, 20, 20);
const QColor OffCellColour(Qt::white);
}

namespace {
QRectF defaultRectFor(const QPointF &position, int columns, int rows)
{
    return QRectF(
        position,
        QSizeF(
            std::max(1, columns) * MileGridConstants::DefaultCellSize,
            std::max(1, rows) * MileGridConstants::DefaultCellSize
            )
        );
}

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

MileGridItem::MileGridItem()
    : m_rect(defaultRectFor(QPointF(0, 0), MileGridConstants::DefaultColumns, MileGridConstants::DefaultRows)),
      m_columns(MileGridConstants::DefaultColumns),
      m_rows(MileGridConstants::DefaultRows),
      m_bits(static_cast<std::size_t>(m_columns * m_rows), false),
      m_bitColour(static_cast<std::size_t>(m_columns * m_rows), MileGridConstants::DefaultCellColour)
{
    initializeHitboxes();
}

MileGridItem::MileGridItem(const MileGridItem &other)
    : CanvasItem(other),
      m_rect(other.m_rect),
      m_columns(other.m_columns),
      m_rows(other.m_rows),
      m_bits(other.m_bits),
      m_bitColour(other.m_bitColour),
      m_viewScale(other.m_viewScale)
{
    initializeHitboxes();
}

MileGridItem::MileGridItem(const QPointF &position, int columns, int rows)
    : MileGridItem(position, columns, rows, MileGridConstants::DefaultCellColour)
{
}

MileGridItem::MileGridItem(const QPointF &position, int columns, int rows, const QColor &cellColor)
    : m_rect(defaultRectFor(position, columns, rows)),
      m_columns(std::max(1, columns)),
      m_rows(std::max(1, rows)),
      m_bits(static_cast<std::size_t>(m_columns * m_rows), false),
      m_bitColour(static_cast<std::size_t>(m_columns * m_rows), cellColor)
{
    MileGridItem::m_currentColour = cellColor;
    initializeHitboxes();
}

MileGridItem::MileGridItem(const QRectF &rect, int columns, int rows, const std::vector<bool> &bits)
    : MileGridItem(rect, columns, rows, bits, MileGridItem::m_currentColour)
{
}

MileGridItem::MileGridItem(const QRectF &rect, int columns, int rows, const std::vector<bool> &bits, const QColor &cellColor)
    : m_rect(rect.normalized()),
      m_columns(std::max(1, columns)),
      m_rows(std::max(1, rows)),
      m_bits(bits),
      m_bitColour(static_cast<std::size_t>(m_columns * m_rows), cellColor)
{
    MileGridItem::m_currentColour = cellColor;
    const std::size_t expectedSize = static_cast<std::size_t>(m_columns * m_rows);
    m_bits.resize(expectedSize, false);
    initializeHitboxes();
}

const std::array<MileGridItem::RoleData, MileGridItem::Role::Count> &MileGridItem::roleData()
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

void MileGridItem::paint(QPainter &painter) const
{
    painter.save();

    painter.fillRect(m_rect, MileGridConstants::OffCellColour);

    const qreal cellWidth = m_rect.width() / m_columns;
    const qreal cellHeight = m_rect.height() / m_rows;

    painter.setPen(Qt::NoPen);
    for (int row = 0; row < m_rows; ++row) {
        for (int column = 0; column < m_columns; ++column) {
            const int index = row * m_columns + column;

            if (!m_bits[static_cast<std::size_t>(index)]) {
                continue;
            }

            painter.setBrush(m_bitColour[index]);
            painter.drawRect(QRectF(
                m_rect.left() + column * cellWidth,
                m_rect.top() + row * cellHeight,
                cellWidth,
                cellHeight
                ));
        }
    }

    QPen gridPen(MileGridConstants::GridLineColour, 1, Qt::SolidLine);
    gridPen.setCosmetic(true);
    painter.setPen(gridPen);
    painter.setBrush(Qt::NoBrush);

    for (int column = 0; column <= m_columns; ++column) {
        const qreal x = m_rect.left() + column * cellWidth;
        painter.drawLine(QPointF(x, m_rect.top()), QPointF(x, m_rect.bottom()));
    }

    for (int row = 0; row <= m_rows; ++row) {
        const qreal y = m_rect.top() + row * cellHeight;
        painter.drawLine(QPointF(m_rect.left(), y), QPointF(m_rect.right(), y));
    }

    painter.restore();

    paintChrome(painter);
}

QRectF MileGridItem::boundingRect() const
{
    return m_rect.united(moveHandleRect()).adjusted(
        -MileGridConstants::HandleSize / 2.0,
        -MileGridConstants::HandleSize / 2.0,
        MileGridConstants::HandleSize / 2.0,
        MileGridConstants::HandleSize / 2.0
        );
}

bool MileGridItem::contains(const QPointF &scenePos) const
{
    return containsGridPoint(scenePos) || moveHandleRect().contains(scenePos);
}

void MileGridItem::moveBy(const QPointF &delta)
{
    m_rect.translate(delta);
    updateHitboxes();
}

void MileGridItem::transformFromRect(const QRectF &oldRect, const QRectF &newRect)
{
    if (oldRect.width() == 0.0 || oldRect.height() == 0.0) {
        return;
    }

    const qreal leftRatio = (m_rect.left() - oldRect.left()) / oldRect.width();
    const qreal topRatio = (m_rect.top() - oldRect.top()) / oldRect.height();
    const qreal rightRatio = (m_rect.right() - oldRect.left()) / oldRect.width();
    const qreal bottomRatio = (m_rect.bottom() - oldRect.top()) / oldRect.height();

    const qreal newLeft = newRect.left() + leftRatio * newRect.width();
    const qreal newTop = newRect.top() + topRatio * newRect.height();
    const qreal newRight = newRect.left() + rightRatio * newRect.width();
    const qreal newBottom = newRect.top() + bottomRatio * newRect.height();

    setRect(QRectF(QPointF(newLeft, newTop), QPointF(newRight, newBottom)));
}

QRectF MileGridItem::rect() const
{
    return m_rect;
}

void MileGridItem::setRect(const QRectF &rect)
{
    m_rect = rect.normalized();
    updateHitboxes();
}

int MileGridItem::columns() const
{
    return m_columns;
}

int MileGridItem::rows() const
{
    return m_rows;
}

const std::vector<bool> &MileGridItem::bits() const
{
    return m_bits;
}

void MileGridItem::setCellColor(const QColor &color)
{
    if (color.isValid()) {
        MileGridItem::m_currentColour = color;
    }
}

bool MileGridItem::toggleCellAt(const QPointF &scenePos)
{
    const int index = cellIndexAt(scenePos);

    if (index < 0) {
        return false;
    }

    std::vector<bool>::reference bit = m_bits[static_cast<std::size_t>(index)];
    bit = !bit;
    m_bitColour[index] = MileGridItem::m_currentColour;
    return true;
}

bool MileGridItem::containsGridPoint(const QPointF &scenePos) const
{
    return m_rect.contains(scenePos);
}

void MileGridItem::setViewScale(qreal zoom)
{
    if (zoom <= 0.0) {
        m_viewScale = 1.0;
    } else {
        m_viewScale = zoom;
    }

    updateHitboxes();
}

std::unique_ptr<CanvasItem> MileGridItem::clone() const
{
    return std::make_unique<MileGridItem>(*this);
}

QJsonObject MileGridItem::serialize(CanvasSerializationContext &context) const
{
    Q_UNUSED(context);

    QJsonArray bits;
    for (bool bit : m_bits) {
        bits.append(bit);
    }

    QJsonArray colors;
    for (const QColor &color : m_bitColour) {
        colors.append(color.name(QColor::HexArgb));
    }

    return QJsonObject {
        {"type", "mileGrid"},
        {"id", QString::number(id())},
        {"rect", rectToJson(m_rect)},
        {"columns", m_columns},
        {"rows", m_rows},
        {"bits", bits},
        {"colors", colors}
    };
}

bool MileGridItem::deserialize(const QJsonObject &json, const CanvasDeserializationContext &context)
{
    Q_UNUSED(context);

    m_rect = rectFromJson(json.value("rect").toObject()).normalized();
    m_columns = std::max(1, json.value("columns").toInt(1));
    m_rows = std::max(1, json.value("rows").toInt(1));

    const std::size_t expectedSize = static_cast<std::size_t>(m_columns * m_rows);
    m_bits.assign(expectedSize, false);
    m_bitColour.assign(expectedSize, MileGridConstants::DefaultCellColour);

    const QJsonArray bits = json.value("bits").toArray();
    for (int i = 0; i < bits.size() && i < static_cast<int>(expectedSize); ++i) {
        m_bits[static_cast<std::size_t>(i)] = bits.at(i).toBool();
    }

    const QJsonArray colors = json.value("colors").toArray();
    for (int i = 0; i < colors.size() && i < static_cast<int>(expectedSize); ++i) {
        const QColor color(colors.at(i).toString());
        if (color.isValid()) {
            m_bitColour[static_cast<std::size_t>(i)] = color;
        }
    }

    initializeHitboxes();
    return true;
}

std::vector<Hitbox*> MileGridItem::hitboxes()
{
    updateHitboxes();

    std::vector<Hitbox*> result;
    result.reserve(Role::Count);

    for (Hitbox &hitbox : m_hitboxes) {
        result.push_back(&hitbox);
    }

    return result;
}

void MileGridItem::onHitboxPressed(int role, const QPointF &scenePos)
{
    if (!isValidRole(role)) {
        return;
    }

    m_lastDragScenePos = scenePos;
}

void MileGridItem::onHitboxDragged(int role, const QPointF &scenePos)
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

void MileGridItem::onHitboxReleased(int role, const QPointF &scenePos)
{
    Q_UNUSED(role);
    Q_UNUSED(scenePos);

    updateHitboxes();
}

bool MileGridItem::isValidRole(int role) const
{
    return role >= 0 && role < Role::Count;
}

bool MileGridItem::isResizeRole(int role) const
{
    return isValidRole(role) && roleData()[role].resize;
}

QPointF MileGridItem::handleCenterForRole(Role role) const
{
    const RoleData &data = roleData()[role];

    return QPointF(
        m_rect.left() + data.xFactor * m_rect.width(),
        m_rect.top() + data.yFactor * m_rect.height()
        );
}

QRectF MileGridItem::handleRectAt(const QPointF &center) const
{
    const qreal sceneSize = MileGridConstants::HandleSize / m_viewScale;

    return QRectF(
        center.x() - sceneSize / 2.0,
        center.y() - sceneSize / 2.0,
        sceneSize,
        sceneSize
        );
}

QRectF MileGridItem::moveHandleRect() const
{
    return QRectF(
        m_rect.left(),
        m_rect.top() - MileGridConstants::MoveHandleHeight,
        m_rect.width(),
        MileGridConstants::MoveHandleHeight
        );
}

void MileGridItem::initializeHitboxes()
{
    for (int role = 0; role < Role::Count; ++role) {
        m_hitboxes[role].owner = this;
        m_hitboxes[role].role = role;
        m_hitboxes[role].cursorShape = roleData()[role].cursorShape;
    }

    updateHitboxes();
}

void MileGridItem::updateHitboxes()
{
    m_hitboxes[Role::MoveHandle].rect = moveHandleRect();

    for (int role = Role::TopLeft; role < Role::Count; ++role) {
        const QPointF center = handleCenterForRole(static_cast<Role>(role));
        m_hitboxes[role].rect = handleRectAt(center);
    }
}

void MileGridItem::resizeFromRole(int role, const QPointF &scenePos)
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

    if (newRect.width() < MileGridConstants::MinGridWidth) {
        newRect.setWidth(MileGridConstants::MinGridWidth);
    }

    if (newRect.height() < MileGridConstants::MinGridHeight) {
        newRect.setHeight(MileGridConstants::MinGridHeight);
    }

    setRect(newRect);
}

void MileGridItem::paintChrome(QPainter &painter) const
{
    painter.save();

    QPen outlinePen(MileGridConstants::OutlineColour, 1, Qt::SolidLine);
    outlinePen.setCosmetic(true);
    painter.setPen(outlinePen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(m_rect);

    painter.setBrush(MileGridConstants::MoveHandleFillColour);
    painter.drawRect(moveHandleRect());

    painter.restore();
}

int MileGridItem::cellIndexAt(const QPointF &scenePos) const
{
    if (!containsGridPoint(scenePos)) {
        return -1;
    }

    const qreal cellWidth = m_rect.width() / m_columns;
    const qreal cellHeight = m_rect.height() / m_rows;

    if (cellWidth <= 0.0 || cellHeight <= 0.0) {
        return -1;
    }

    int column = static_cast<int>((scenePos.x() - m_rect.left()) / cellWidth);
    int row = static_cast<int>((scenePos.y() - m_rect.top()) / cellHeight);

    column = std::clamp(column, 0, m_columns - 1);
    row = std::clamp(row, 0, m_rows - 1);

    return row * m_columns + column;
}

void MileGridItem::setCurrentColour(const QColor& color) {
    m_currentColour = color;
}
