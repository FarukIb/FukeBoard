#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include <QWidget>
#include "appconstants.h"
#include "canvasitem.h"
#include "hitbox.h"
#include "selectioncontroller.h"

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

    std::vector<HitboxOwner*> m_hitboxOwners;
    std::optional<Hitbox*> m_activeHitbox;

    std::vector<std::unique_ptr<CanvasItem>> m_items;

    SelectionController m_selection;

    QColor m_penColor = AppConstants::DefaultPenColor;

    QPainterPath m_currentPath;
    bool m_drawing = false;

    QRectF selectedItemsBoundingRect() const;
    void selectItemsInsideRect(const QRectF &rect);

    bool m_panning = false;
    QPointF m_lastPanScreenPos;

    QPointF m_offset = QPointF(0, 0);
    double m_zoom = 1.0;

    QPointF screenToScene(const QPointF &screenPoint) const;
    QPointF sceneToScreen(const QPointF &scenePoint) const;

    CanvasItem* itemAt(const QPointF &scenePos) const;

    Hitbox *hitboxAt(const QPointF &scenePos) const;
    void registerHitboxOwner(HitboxOwner *owner);
    void clearActiveHitboxIfOwnedBy(HitboxOwner *owner);

    void handleHitboxPress(Hitbox *hitbox, const QPointF &scenePos);
    bool handleActiveHitboxDrag(const QPointF &scenePos);
    bool handleActiveHitboxRelease(const QPointF &scenePos);

    void eraseAt(const QPointF &scenePos);
    void createTextEditorAt(const QPointF &scenePos);

    // Paint helpers
    void drawGrid(QPainter &painter, const QRectF &visibleScene);
    void drawItems(QPainter &painter);
    void drawCurrentStroke(QPainter &painter);
    void drawSelection(QPainter &painter);

    // Mouse press handlers
    void startPanning(const QPointF &screenPos);
    void startPenStroke(const QPointF &scenePos);
    void handleSelectPress(const QPointF &scenePos);

    // Mouse move handlers
    void continuePanning(const QPointF &screenPos);
    void continuePenStroke(const QPointF &scenePos);
    void continueErasing(const QPointF &scenePos);
    void moveSelection(const QPointF &scenePos);
    void updateSelectionRectangle(const QPointF &scenePos);

    // Mouse release handlers
    void finishPanning();
    void finishPenStroke();
    void finishSelection();

    // Other input helpers
    void handleZoom(const QPointF &screenPos, int wheelDelta);
};

#endif // CANVASWIDGET_H
