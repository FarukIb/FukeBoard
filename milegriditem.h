#ifndef MILEGRIDITEM_H
#define MILEGRIDITEM_H

#include "canvasitem.h"

#include <QColor>
#include <QPointF>
#include <QRectF>
#include <Qt>

#include <array>
#include <memory>
#include <vector>

class MileGridItem : public CanvasItem
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

    MileGridItem();
    MileGridItem(const MileGridItem &other);
    MileGridItem(const QPointF &position, int columns, int rows);
    MileGridItem(const QPointF &position, int columns, int rows, const QColor &cellColor);
    MileGridItem(const QRectF &rect, int columns, int rows, const std::vector<bool> &bits);
    MileGridItem(const QRectF &rect, int columns, int rows, const std::vector<bool> &bits, const QColor &cellColor);

    void paint(QPainter &painter) const override;
    QRectF boundingRect() const override;
    bool contains(const QPointF &scenePos) const override;
    void moveBy(const QPointF &delta) override;
    void transformFromRect(const QRectF &oldRect, const QRectF &newRect) override;

    QRectF rect() const;
    void setRect(const QRectF &rect);

    int columns() const;
    int rows() const;
    const std::vector<bool> &bits() const;
    QColor cellColor() const;
    void setCellColor(const QColor &color);

    bool toggleCellAt(const QPointF &scenePos);
    bool containsGridPoint(const QPointF &scenePos) const;

    void setViewScale(qreal zoom);

    std::unique_ptr<CanvasItem> clone() const override;
    QJsonObject serialize(CanvasSerializationContext &context) const override;
    bool deserialize(const QJsonObject &json, const CanvasDeserializationContext &context) override;

    std::vector<Hitbox*> hitboxes() override;
    void onHitboxPressed(int role, const QPointF &scenePos) override;
    void onHitboxDragged(int role, const QPointF &scenePos) override;
    void onHitboxReleased(int role, const QPointF &scenePos) override;

    static void setCurrentColour(const QColor& col);
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
    int cellIndexAt(const QPointF &scenePos) const;

    QRectF m_rect;
    int m_columns = 1;
    int m_rows = 1;
    std::vector<bool> m_bits;
    std::vector<QColor> m_bitColour;
    inline static QColor m_currentColour = QColor(20, 20, 20);

    qreal m_viewScale = 1.0;

    QPointF m_lastDragScenePos;
    std::array<Hitbox, Role::Count> m_hitboxes;
};

#endif // MILEGRIDITEM_H
