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
        Body = 1
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

    void onHitboxPressed(int role, const QPointF &scenePos) override;
    void onHitboxDragged(int role, const QPointF &scenePos) override;
    void onHitboxReleased(int role, const QPointF &scenePos) override;

private:
    void updateHitboxes();

    std::set<CanvasItem*> m_selectedItems;

    bool m_drawingSelectionRectangle = false;

    QPointF m_selectionStartScenePos;
    QRectF m_selectionRect;

    QPointF m_lastDragSceenPos;

    QPointF m_lastDragScenePos;

    Hitbox m_bodyHitbox;
};

#endif // SELECTIONCONTROLLER_H
