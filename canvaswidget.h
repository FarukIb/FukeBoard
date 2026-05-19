#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include <QWidget>
#include "appconstants.h"
#include "canvasitem.h"

class CanvasWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CanvasWidget(QWidget *parent = nullptr);

    void setMode(Mode mode);
    void setPenColor(const QColor &color);
    void setPenWidth(int width);

    void deleteSelection();
    void duplicateSelection();
protected:
    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *EVENT) override;

    void wheelEvent(QWheelEvent *event) override;

private:
    int m_penWidth = AppConstants::DefaultPenWidth;
    Mode m_mode = Mode::Pen;

    std::vector<std::unique_ptr<CanvasItem>> m_items;

    QColor m_penColor = AppConstants::DefaultPenColor;

    QPainterPath m_currentPath;
    bool m_drawing = false;

    std::set<CanvasItem*> m_selectedItems;
    bool m_draggingSelection = false;
    QPointF m_lastDragScenePos;

    bool m_panning = false;
    QPointF m_lastPanScreenPos;

    QPointF m_offset = QPointF(0, 0);
    double m_zoom = 1.0;

    QPointF screenToScene(const QPointF &screenPoint) const;
    QPointF sceneToScreen(const QPointF &scenePoint) const;

    CanvasItem* itemAt(const QPointF &scenePos) const;

    void eraseAt(const QPointF &scenePos);
    void createTextEditorAt(const QPointF &scenePos);
};

#endif // CANVASWIDGET_H
