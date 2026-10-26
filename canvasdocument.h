#ifndef CANVASDOCUMENT_H
#define CANVASDOCUMENT_H

#include "canvasitem.h"

// Owns canvas items and the document-level invariants: stable IDs and z-order.
// It deliberately has no QWidget or input-event dependency.
class CanvasDocument
{
public:
    const CanvasItemList &items() const;
    CanvasItemList &items();

    CanvasItem &addItem(std::unique_ptr<CanvasItem> item);
    void assignFreshId(CanvasItem &item);
    void replaceItems(CanvasItemList items);
    void restoreSnapshot(const CanvasItemList &snapshot);

    CanvasItem *itemAt(const QPointF &scenePos) const;
    CanvasItem *itemById(CanvasItemId id) const;
    void bringToFront(CanvasItem *item);

    QRectF boundingRect() const;
    CanvasItemList cloneItems() const;
    bool snapshotsEquivalent(const CanvasItemList &left, const CanvasItemList &right) const;

private:
    void updateNextItemId();

    CanvasItemList m_items;
    CanvasItemId m_nextItemId = 1;
};

#endif // CANVASDOCUMENT_H
