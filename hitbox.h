#ifndef HITBOX_H
#define HITBOX_H

#include <QPointF>
#include <QRectF>

class HitboxOwner;

struct Hitbox {
    QRectF rect;
    int role;
    HitboxOwner *owner = nullptr;

    bool contains(const QPointF &scenePos) const {
        return rect.contains(scenePos);
    }
};

class HitboxOwner {
public:
    virtual ~HitboxOwner() = default;

    virtual void onHitboxPressed(int role, const QPointF &scenePos) = 0;
    virtual void onHitboxDragged(int role, const QPointF &scenePos) = 0;
    virtual void onHitboxReleased(int role, const QPointF &scenePos) = 0;
};

#endif // HITBOX_H