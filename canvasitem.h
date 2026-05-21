#ifndef CANVASITEM_H
#define CANVASITEM_H

#include <QPainter>
#include <QPainterPath>
#include <memory>
#include "hitbox.h"

class CanvasItem : public HitboxOwner
{
public:
    virtual ~CanvasItem() = default;

    virtual void paint(QPainter &painter) const = 0;
    virtual QRectF boundingRect() const = 0;
    virtual bool contains(const QPointF &scenePos) const = 0;
    virtual void moveBy(const QPointF &delta) = 0;
    virtual void transformFromRect(const QRectF &oldRect, const QRectF &newRect) = 0;

    std::vector<Hitbox*> hitboxes() override;

    void onHitboxPressed(int role, const QPointF &scenePos) override;
    void onHitboxDragged(int role, const QPointF &scenePos) override;
    void onHitboxReleased(int role, const QPointF &scenePos) override;

    virtual std::unique_ptr<CanvasItem> clone() const = 0;
};

class StrokeItem : public CanvasItem
{
public:
    StrokeItem() = default;

    StrokeItem(const QPainterPath &path, const QColor &color, int width);

    void paint(QPainter &painter) const override;
    QRectF boundingRect() const override;
    bool contains(const QPointF &scenePos) const override;
    void moveBy(const QPointF &delta) override;
    void transformFromRect(const QRectF &oldRect, const QRectF &newRect) override;

    virtual std::unique_ptr<CanvasItem> clone() const override;
private:
    QPainterPath m_path;
    QColor m_color = Qt::black;
    int m_width = 3;
};


/*
 all of this should be very much reworked right not its too simple
i just want a proof of concept
*/
class TextBoxItem : public CanvasItem
{
public:
    TextBoxItem() = default;

    TextBoxItem(const QPointF &position, const QString &text)
        : m_rect(position, QSizeF(260, 60)), m_text(text)
    {
    }

    TextBoxItem(const QRectF &rect, const QString &text)
        : m_rect(rect), m_text(text)
    {
    }

    void paint(QPainter &painter) const override
    {
        painter.setPen(Qt::black);
        painter.setBrush(Qt::NoBrush);

        QFont font = painter.font();
        font.setPointSize(14);
        painter.setFont(font);

        painter.drawText(boundingRect(), Qt::AlignLeft | Qt::AlignTop, m_text);
    }

    QRectF boundingRect() const override
    {
        return m_rect;
    }

    bool contains(const QPointF &scenePos) const override
    {
        return boundingRect().contains(scenePos);
    }

    void moveBy(const QPointF &delta) override
    {
        m_rect.translate(delta);
    }

    void transformFromRect(const QRectF &oldRect, const QRectF &newRect) override
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

        m_rect = QRectF(
                     QPointF(newLeft, newTop),
                     QPointF(newRight, newBottom)
                     ).normalized();
    }

    std::unique_ptr<CanvasItem> clone() const override
    {
        return std::make_unique<TextBoxItem>(m_rect, m_text);
    }

private:
    QRectF m_rect;
    QString m_text;
};

#endif // CANVASITEM_H
