#ifndef CANVASITEM_H
#define CANVASITEM_H

#include <QPainter>
#include <QPainterPath>
#include <memory>

class CanvasItem
{
public:
    virtual ~CanvasItem() = default;

    virtual void paint(QPainter &painter) const = 0;
    virtual QRectF boundingRect() const = 0;
    virtual bool contains(const QPointF &scenePos) const = 0;
    virtual void moveBy(const QPointF &delta) = 0;

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
    virtual void moveBy(const QPointF &delta) override;

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
        : m_position(position), m_text(text)
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
        return QRectF(m_position, QSizeF(260, 60));
    }

    bool contains(const QPointF &scenePos) const override
    {
        return boundingRect().contains(scenePos);
    }

    void moveBy(const QPointF &delta) override
    {
        m_position += delta;
    }

    std::unique_ptr<CanvasItem> clone() const override
    {
        return std::make_unique<TextBoxItem>(m_position, m_text);
    }

private:
    QPointF m_position;
    QString m_text;
};

#endif // CANVASITEM_H
