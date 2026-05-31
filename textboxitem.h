#ifndef TEXTBOXITEM_H
#define TEXTBOXITEM_H

#include "canvasitem.h"

#include <QFont>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <Qt>

#include <array>
#include <memory>
#include <vector>

class TextBoxItem : public CanvasItem
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

    TextBoxItem();
    TextBoxItem(const TextBoxItem &other);
    TextBoxItem(const QPointF &position, const QString &html);
    TextBoxItem(const QRectF &rect, const QString &html);

    void paint(QPainter &painter) const override;
    void paintChrome(QPainter &painter) const;
    QRectF boundingRect() const override;
    bool contains(const QPointF &scenePos) const override;
    void moveBy(const QPointF &delta) override;
    void transformFromRect(const QRectF &oldRect, const QRectF &newRect) override;

    QRectF rect() const;
    void setRect(const QRectF &rect);

    QString html() const;
    void setHtml(const QString &html);
    QString plainText() const;

    static QFont textFont();
    void setViewScale(qreal zoom);

    std::unique_ptr<CanvasItem> clone() const override;

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

    QRectF m_rect;
    QString m_html;
    qreal m_viewScale = 1.0;

    QPointF m_lastDragScenePos;
    std::array<Hitbox, Role::Count> m_hitboxes;
};

#endif // TEXTBOXITEM_H
