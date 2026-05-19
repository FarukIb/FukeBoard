#include "canvaswidget.h"

#include <QMouseEvent>
#include <QLineEdit>

namespace {
constexpr int DeltaXAfterCopy = 30;
constexpr int DeltaYAfterCopy = 30;

const QColor BackgroundColour = Qt::white;

const QColor GridColour = QColor(230,230,230);
constexpr int GridSize = 50;

const QColor SelectionLinesColour = Qt::blue;
const Qt::PenStyle SelectionLineType = Qt::DashLine;
const int SelectionLineWidth = 1;
const QColor SelectionRectangleColour = QColor(0, 120, 215, 40);

const double ZoomFactor = 1.15;
const double MinZoom = 0.2;
const double MaxZoom = 5.0;
}

CanvasWidget::CanvasWidget(QWidget *parent)
    : QWidget{parent}
{
    setAttribute(Qt::WA_StaticContents);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet("background-color: white;");
}

void CanvasWidget::setMode(Mode mode) {
    m_mode = mode;
    m_selectedItems = std::set<CanvasItem*>();
    m_draggingSelection = false;
    update();
}

void CanvasWidget::setPenColor(const QColor &color) {
    m_penColor = color;
}

void CanvasWidget::setPenWidth(int width) {
    m_penWidth = width;
}

QRectF CanvasWidget::selectedItemsBoundingRect() const {
    if (m_selectedItems.empty()) {
        return QRectF();
    }

    QRectF result = (*m_selectedItems.begin())->boundingRect();

    for (CanvasItem *item : m_selectedItems) {
        result = result.united(item->boundingRect());
    }

    return result;
}

void CanvasWidget::selectItemsInsideRect(const QRectF &rect) {
    m_selectedItems.clear();

    QRectF normalizedRect = rect.normalized();

    for (const auto &item : m_items) {
        // right now using intersects() because it feels better for freehand strokes
        if (normalizedRect.intersects(item->boundingRect())) {
            m_selectedItems.insert(item.get());
        }
    }
}

void CanvasWidget::deleteSelection() {
    if (m_selectedItems.empty()) {
        return;
    }

    for (auto it = m_items.begin(); it != m_items.end(); ) {
        if (m_selectedItems.count(it->get())) {
            it = m_items.erase(it);
        } else {
            ++it;
        }
    }

    m_selectedItems.clear();
    update();
}

void CanvasWidget::duplicateSelection() {
    if (m_selectedItems.empty()) {
        return;
    }

    std::set<CanvasItem*> copied;
    for (auto item : m_selectedItems) {
        auto newItem = item->clone();
        newItem->moveBy(QPointF(DeltaXAfterCopy, DeltaYAfterCopy));

        copied.insert(newItem.get());
        m_items.push_back(std::move(newItem));
    }

    m_selectedItems = copied;
    update();
}

QPointF CanvasWidget::screenToScene(const QPointF &screenPoint) const {
    return (screenPoint - m_offset) / m_zoom;
}

QPointF CanvasWidget::sceneToScreen(const QPointF &screenPoint) const {
    return screenPoint * m_zoom + m_offset;
}

CanvasItem *CanvasWidget::itemAt(const QPointF &scenePos) const {
    if (m_items.empty())
        return nullptr;
    for (int i = m_items.size() - 1; i >= 0; i--)
        if (m_items[i]->contains(scenePos))
            return m_items[i].get();

    return nullptr;
}

void CanvasWidget::drawGrid(QPainter &painter, const QRectF &visibleScene) {
    QPen gridPen(GridColour);
    gridPen.setWidthF(0);
    painter.setPen(gridPen);

    int left = static_cast<int>(visibleScene.left()) - GridSize;
    int right = static_cast<int>(visibleScene.right()) + GridSize;
    int top = static_cast<int>(visibleScene.top()) - GridSize;
    int bottom = static_cast<int>(visibleScene.bottom()) + GridSize;

    for (int x = left - left % GridSize; x < right; x += GridSize) {
        painter.drawLine(QPointF(x, top), QPointF(x, bottom));
    }

    for (int y = top - top % GridSize; y < bottom; y += GridSize) {
        painter.drawLine(QPointF(left, y), QPointF(right, y));
    }
}

void CanvasWidget::drawItems(QPainter &painter) {
    for (const auto &item : m_items) {
        item->paint(painter);
    }
}

void CanvasWidget::drawCurrentStroke(QPainter &painter) {
    if (!m_drawing) {
        return;
    }

    QPen pen(m_penColor, m_penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(m_currentPath);
}

void CanvasWidget::drawSelectionRectangle(QPainter &painter) {
    if (!m_drawingSelectionRect) {
        return;
    }

    QPen selectionPen(SelectionLinesColour, SelectionLineWidth, SelectionLineType);
    selectionPen.setCosmetic(true);

    painter.setPen(selectionPen);
    painter.setBrush(SelectionRectangleColour);
    painter.drawRect(m_selectionRect.normalized());
}

void CanvasWidget::drawSelectionBorder(QPainter &painter) {
    if (m_selectedItems.empty()) {
        return;
    }

    QPen selectionPen(SelectionLinesColour, SelectionLineWidth, SelectionLineType);
    selectionPen.setCosmetic(true);

    painter.setPen(selectionPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(selectedItemsBoundingRect());
}

void CanvasWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.fillRect(rect(), BackgroundColour);

    painter.translate(m_offset);
    painter.scale(m_zoom, m_zoom);

    QRectF visibleScene(
        screenToScene(QPointF(0, 0)),
        screenToScene(QPointF(width(), height()))
        );

    drawGrid(painter, visibleScene);
    drawItems(painter);
    drawCurrentStroke(painter);
    drawSelectionRectangle(painter);
    drawSelectionBorder(painter);
}

void CanvasWidget::startPanning(const QPointF &screenPos) {
    m_panning = true;
    m_lastPanScreenPos = screenPos;
    setCursor(Qt::ClosedHandCursor);
}

void CanvasWidget::startPenStroke(const QPointF &scenePos) {
    m_selectedItems.clear();

    m_drawing = true;
    m_currentPath = QPainterPath();
    m_currentPath.moveTo(scenePos);

    update();
}

void CanvasWidget::handleSelectPress(const QPointF &scenePos) {
    QRectF currentSelectionBounds = selectedItemsBoundingRect();

    if (!m_selectedItems.empty() && currentSelectionBounds.contains(scenePos)) {
        m_draggingSelection = true;
        m_drawingSelectionRect = false;
        m_lastDragScenePos = scenePos;
    } else {
        m_selectedItems.clear();
        m_draggingSelection = false;
        m_drawingSelectionRect = true;
        m_selectionStartScenePos = scenePos;
        m_selectionRect = QRectF(scenePos, scenePos);
    }

    update();
}

void CanvasWidget::eraseAt(const QPointF &scenePos) {
    if (m_items.empty())
        return;
    for (int i = m_items.size() - 1; i >= 0; i--) {
        if (m_items[i]->contains(scenePos)) {
            if (m_selectedItems.count(m_items[i].get())) {
                m_selectedItems.erase(m_items[i].get());
            }

            m_items.erase(m_items.begin() + i);
            update();
            return;
        }
    }
}

void CanvasWidget::mousePressEvent(QMouseEvent *event) {
    const QPointF scenePos = screenToScene(event->position());

    if (event->button() == Qt::MiddleButton) {
        startPanning(event->position());
        return;
    }

    if (event->button() != Qt::LeftButton) {
        return;
    }

    switch (m_mode) {
    case Mode::Pen:
        startPenStroke(scenePos);
        return;

    case Mode::Erase:
        eraseAt(scenePos);
        return;

    case Mode::Select:
        handleSelectPress(scenePos);
        return;
    }
}

void CanvasWidget::continuePanning(const QPointF &screenPos) {
    QPointF delta = screenPos - m_lastPanScreenPos;
    m_offset += delta;
    m_lastPanScreenPos = screenPos;
    update();
}

void CanvasWidget::continuePenStroke(const QPointF &scenePos) {
    m_currentPath.lineTo(scenePos);
    update();
}

void CanvasWidget::continueErasing(const QPointF &scenePos) {
    eraseAt(scenePos);
}

void CanvasWidget::moveSelection(const QPointF &scenePos) {
    QPointF delta = scenePos - m_lastDragScenePos;

    for (CanvasItem *item : m_selectedItems) {
        item->moveBy(delta);
    }

    m_lastDragScenePos = scenePos;
    update();
}

void CanvasWidget::updateSelectionRectangle(const QPointF &scenePos) {
    m_selectionRect = QRectF(m_selectionStartScenePos, scenePos).normalized();
    update();
}

void CanvasWidget::mouseMoveEvent(QMouseEvent *event) {
    const QPointF scenePos = screenToScene(event->position());

    if (m_panning) {
        continuePanning(event->position());
        return;
    }

    switch (m_mode) {
    case Mode::Pen:
        if (m_drawing) {
            continuePenStroke(scenePos);
        }
        return;

    case Mode::Erase:
        if (event->buttons() & Qt::LeftButton) {
            continueErasing(scenePos);
        }
        return;

    case Mode::Select:
        if (m_draggingSelection && !m_selectedItems.empty()) {
            moveSelection(scenePos);
        } else if (m_drawingSelectionRect) {
            updateSelectionRectangle(scenePos);
        }
        return;
    }
}

void CanvasWidget::finishPanning() {
    m_panning = false;
    unsetCursor();
}

void CanvasWidget::finishPenStroke() {
    m_drawing = false;

    auto stroke = std::make_unique<StrokeItem>(
        m_currentPath,
        m_penColor,
        m_penWidth
        );

    m_items.push_back(std::move(stroke));

    m_currentPath = QPainterPath();
    update();
}

void CanvasWidget::finishSelection() {
    if (m_drawingSelectionRect) {
        selectItemsInsideRect(m_selectionRect);
        m_drawingSelectionRect = false;
        m_selectionRect = QRectF();
    }

    m_draggingSelection = false;
    update();
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton ||
        (event->button() == Qt::LeftButton && m_panning)) {
        finishPanning();
        return;
    }

    if (event->button() != Qt::LeftButton) {
        return;
    }

    switch (m_mode) {
    case Mode::Pen:
        if (m_drawing) {
            finishPenStroke();
        }
        return;

    case Mode::Erase:
        return;

    case Mode::Select:
        finishSelection();
        return;
    }
}
void CanvasWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QPointF scenePos = screenToScene(event->position());

    if (event->button() == Qt::LeftButton) {
        createTextEditorAt(scenePos);
    }
}

void CanvasWidget::handleZoom(const QPointF &screenPos, int wheelDelta) {
    QPointF mouseSceneBeforeZoom = screenToScene(screenPos);

    if (wheelDelta > 0) {
        m_zoom *= ZoomFactor;
    } else {
        m_zoom /= ZoomFactor;
    }

    m_zoom = std::clamp(m_zoom, MinZoom, MaxZoom);

    QPointF mouseSceneAfterZoom = screenToScene(screenPos);

    m_offset += (mouseSceneAfterZoom - mouseSceneBeforeZoom) * m_zoom;

    update();
}

void CanvasWidget::wheelEvent(QWheelEvent *event)
{
    handleZoom(event->position(), event->angleDelta().y());
}

// this is temporary right now
void CanvasWidget::createTextEditorAt(const QPointF &scenePos)
{
    QPointF screenPos = sceneToScreen(scenePos);

    auto *editor = new QLineEdit(this);
    editor->move(screenPos.toPoint());
    editor->resize(260, 32);
    editor->show();
    editor->setFocus();

    connect(editor, &QLineEdit::editingFinished, this, [this, editor, scenePos]() {
        QString text = editor->text().trimmed();

        if (!text.isEmpty()) {
            auto item = std::make_unique<TextBoxItem>(scenePos, text);
            m_items.push_back(std::move(item));
        }

        editor->deleteLater();
        update();
    });
}