#ifndef SELECTIONCONTROLLER_H
#define SELECTIONCONTROLLER_H

#include "canvasitem.h"
#include "hitbox.h"

#include <QPainter>
#include <QPointF>
#include <QRectF>

#include <memory>
#include <set>
#include <vector>

class SelectionController : public HitboxOwner
{
public:
    enum Role {
        Body = 0,

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

    SelectionController();

    void clear();
    bool empty() const;

    const std::set<CanvasItem*> &selectedItems() const;

    void beginSelectionRectangle(const QPointF &scenePos);
    void updateSelectionRectangle(const QPointF &scenePos);
    void finishSelectionRectangle(const std::vector<std::unique_ptr<CanvasItem>> &items);

    bool isDrawingSelectionRectangle() const;

    void selectItemsInsideRect(
        const QRectF &rect,
        const std::vector<std::unique_ptr<CanvasItem>> &items
        );

    void deleteSelectedItems(std::vector<std::unique_ptr<CanvasItem>> &items);

    void duplicateSelectedItems(std::vector<std::unique_ptr<CanvasItem>> &items, const QPointF &copyOffset);

    QRectF boundingRect() const;

    void paint(QPainter &painter) const;

    std::vector<Hitbox*> hitboxes() override;

    void setViewScale(qreal zoom);

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

    QPointF handleCenterForRole(Role role, const QRectF &bounds) const;
    QRectF handleRectAt(const QPointF &center) const;

    void initializeHitboxes();
    void updateHitboxes();

    void moveSelectedItems(const QPointF &scenePos);
    void resizeSelectedItems(int role, const QPointF &scenePos);
    QRectF resizedRectForRole(int role, const QPointF &scenePos) const;

    std::vector<Hitbox*> visibleResizeHitboxes();

    std::set<CanvasItem*> m_selectedItems;

    bool m_drawingSelectionRectangle = false;
    qreal m_viewScale;

    QRectF m_resizeStartBounds;
    QPointF m_selectionStartScenePos;
    QRectF m_selectionRect;

    QPointF m_lastDragSceenPos;

    QPointF m_lastDragScenePos;

    Hitbox m_bodyHitbox;

    std::array<Hitbox, Role::Count> m_hitboxes;
};

#endif // SELECTIONCONTROLLER_H
