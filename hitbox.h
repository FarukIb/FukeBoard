#ifndef HITBOX_H
#define HITBOX_H

#include <QCursor>
#include <QPointF>
#include <QRectF>

#include <vector>

class HitboxOwner;

struct Hitbox {
    QRectF rect;
    int role = 0;
    HitboxOwner *owner = nullptr;
    Qt::CursorShape cursorShape = Qt::ArrowCursor;

    bool contains(const QPointF &scenePos) const {
        return rect.contains(scenePos);
    }

    QCursor cursor() const {
        return QCursor(cursorShape);
    }
};

class HitboxOwner {
public:
    virtual ~HitboxOwner() = default;

    virtual std::vector<Hitbox*> hitboxes() = 0;

    virtual void onHitboxPressed(int role, const QPointF &scenePos) = 0;
    virtual void onHitboxDragged(int role, const QPointF &scenePos) = 0;
    virtual void onHitboxReleased(int role, const QPointF &scenePos) = 0;
};

#endif // HITBOX_H