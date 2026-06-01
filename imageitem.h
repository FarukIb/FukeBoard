#ifndef IMAGEITEM_H
#define IMAGEITEM_H

#include "canvasitem.h"

#include <QImage>
#include <QPointF>
#include <QRectF>
#include <Qt>

#include <array>
#include <memory>
#include <vector>

class ImageItem : public CanvasItem
{
public:
    enum Role {
        MoveHandle = 0,

        TopLeft,
        Top,
        TopRight,
        Right,
        BottomRight,
        Bottom,
        BottomLeft,
        Left,

        Count
    };

    ImageItem();
    ImageItem(const ImageItem &other);
    ImageItem(const QRectF &rect, const QImage &image);

    void paint(QPainter &painter) const override;
    QRectF boundingRect() const override;
    bool contains(const QPointF &scenePos) const override;
    void moveBy(const QPointF &delta) override;
    void transformFromRect(const QRectF &oldRect, const QRectF &newRect) override;

    QRectF rect() const;
    void setRect(const QRectF &rect);

    const QImage &image() const;
    void setImage(const QImage &image);

    void setViewScale(qreal zoom);

    std::unique_ptr<CanvasItem> clone() const override;
    QJsonObject serialize(CanvasSerializationContext &context) const override;
    bool deserialize(const QJsonObject &json, const CanvasDeserializationContext &context) override;

    std::vector<Hitbox*> hitboxes() override;
    void onHitboxPressed(int role, const QPointF &scenePos) override;
    void onHitboxDragged(int role, const QPointF &scenePos) override;
    void onHitboxReleased(int role, const QPointF &scenePos) override;

private:
    struct RoleData {
        bool resize = false;

        qreal xFactor = 0.0;
        qreal yFactor = 0.0;

        bool changesLeft = false;
        bool changesTop = false;
        bool changesRight = false;
        bool changesBottom = false;

        Qt::CursorShape cursorShape = Qt::ArrowCursor;
    };

    static const std::array<RoleData, Role::Count> &roleData();

    bool isValidRole(int role) const;
    bool isResizeRole(int role) const;
    QPointF handleCenterForRole(Role role) const;
    QRectF handleRectAt(const QPointF &center) const;
    QRectF moveHandleRect() const;
    void initializeHitboxes();
    void updateHitboxes();
    void resizeFromRole(int role, const QPointF &scenePos);
    void paintChrome(QPainter &painter) const;

    QRectF m_rect;
    QImage m_image;
    qreal m_viewScale = 1.0;

    QPointF m_lastDragScenePos;
    std::array<Hitbox, Role::Count> m_hitboxes;
};

#endif // IMAGEITEM_H
