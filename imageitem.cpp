#include "imageitem.h"

#include <QPainter>
#include <QPen>

namespace {
constexpr qreal HandleSize = 20.0;
constexpr qreal MoveHandleHeight = 18.0;
constexpr qreal MinImageWidth = 20.0;
constexpr qreal MinImageHeight = 20.0;
const QColor ImageOutlineColour(0, 120, 215);
const QColor MoveHandleFillColour(0, 120, 215, 70);
}

ImageItem::ImageItem()
{
    initializeHitboxes();
}

ImageItem::ImageItem(const ImageItem &other)
    : CanvasItem(other),
      m_rect(other.m_rect),
      m_image(other.m_image),
      m_viewScale(other.m_viewScale)
{
    initializeHitboxes();
}

ImageItem::ImageItem(const QRectF &rect, const QImage &image)
    : m_rect(rect), m_image(image)
{
    initializeHitboxes();
}

const std::array<ImageItem::RoleData, ImageItem::Role::Count> &ImageItem::roleData()
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

void ImageItem::paint(QPainter &painter) const
{
    if (!m_image.isNull()) {
        painter.drawImage(m_rect, m_image);
    }

    paintChrome(painter);
}

QRectF ImageItem::boundingRect() const
{
    return m_rect.united(moveHandleRect()).adjusted(
        -HandleSize / 2.0,
        -HandleSize / 2.0,
        HandleSize / 2.0,
        HandleSize / 2.0
        );
}

bool ImageItem::contains(const QPointF &scenePos) const
{
    return m_rect.contains(scenePos) || moveHandleRect().contains(scenePos);
}

void ImageItem::moveBy(const QPointF &delta)
{
    m_rect.translate(delta);
    updateHitboxes();
}

void ImageItem::transformFromRect(const QRectF &oldRect, const QRectF &newRect)
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

    setRect(QRectF(QPointF(newLeft, newTop), QPointF(newRight, newBottom)));
}

QRectF ImageItem::rect() const
{
    return m_rect;
}

void ImageItem::setRect(const QRectF &rect)
{
    m_rect = rect.normalized();
    updateHitboxes();
}

const QImage &ImageItem::image() const
{
    return m_image;
}

void ImageItem::setImage(const QImage &image)
{
    m_image = image;
}

void ImageItem::setViewScale(qreal zoom)
{
    if (zoom <= 0.0) {
        m_viewScale = 1.0;
    } else {
        m_viewScale = zoom;
    }

    updateHitboxes();
}

std::unique_ptr<CanvasItem> ImageItem::clone() const
{
    return std::make_unique<ImageItem>(*this);
}

std::vector<Hitbox*> ImageItem::hitboxes()
{
    updateHitboxes();

    std::vector<Hitbox*> result;
    result.reserve(Role::Count);

    for (Hitbox &hitbox : m_hitboxes) {
        result.push_back(&hitbox);
    }

    return result;
}

void ImageItem::onHitboxPressed(int role, const QPointF &scenePos)
{
    if (!isValidRole(role)) {
        return;
    }

    m_lastDragScenePos = scenePos;
}

void ImageItem::onHitboxDragged(int role, const QPointF &scenePos)
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

void ImageItem::onHitboxReleased(int role, const QPointF &scenePos)
{
    Q_UNUSED(role);
    Q_UNUSED(scenePos);

    updateHitboxes();
}

bool ImageItem::isValidRole(int role) const
{
    return role >= 0 && role < Role::Count;
}

bool ImageItem::isResizeRole(int role) const
{
    return isValidRole(role) && roleData()[role].resize;
}

QPointF ImageItem::handleCenterForRole(Role role) const
{
    const RoleData &data = roleData()[role];

    return QPointF(
        m_rect.left() + data.xFactor * m_rect.width(),
        m_rect.top() + data.yFactor * m_rect.height()
        );
}

QRectF ImageItem::handleRectAt(const QPointF &center) const
{
    const qreal sceneSize = HandleSize / m_viewScale;

    return QRectF(
        center.x() - sceneSize / 2.0,
        center.y() - sceneSize / 2.0,
        sceneSize,
        sceneSize
        );
}

QRectF ImageItem::moveHandleRect() const
{
    return QRectF(
        m_rect.left(),
        m_rect.top() - MoveHandleHeight,
        m_rect.width(),
        MoveHandleHeight
        );
}

void ImageItem::initializeHitboxes()
{
    for (int role = 0; role < Role::Count; ++role) {
        m_hitboxes[role].owner = this;
        m_hitboxes[role].role = role;
        m_hitboxes[role].cursorShape = roleData()[role].cursorShape;
    }

    updateHitboxes();
}

void ImageItem::updateHitboxes()
{
    m_hitboxes[Role::MoveHandle].rect = moveHandleRect();

    for (int role = Role::TopLeft; role < Role::Count; ++role) {
        const QPointF center = handleCenterForRole(static_cast<Role>(role));
        m_hitboxes[role].rect = handleRectAt(center);
    }
}

void ImageItem::resizeFromRole(int role, const QPointF &scenePos)
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

    if (newRect.width() < MinImageWidth) {
        newRect.setWidth(MinImageWidth);
    }

    if (newRect.height() < MinImageHeight) {
        newRect.setHeight(MinImageHeight);
    }

    setRect(newRect);
}

void ImageItem::paintChrome(QPainter &painter) const
{
    painter.save();

    QPen outlinePen(ImageOutlineColour, 1, Qt::SolidLine);
    outlinePen.setCosmetic(true);
    painter.setPen(outlinePen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(m_rect);

    painter.setBrush(MoveHandleFillColour);
    painter.drawRect(moveHandleRect());

    painter.restore();
}
