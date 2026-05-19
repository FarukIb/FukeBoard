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


    // draw the grid
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

    // draw items
    // maybe decide to not paint if not in frame, hopefully wont be an issue
    for (const auto &item : m_items) {
        item->paint(painter);
    }

    // draw the temporary line we are currently drawing
    if (m_drawing) {
        QPen pen(m_penColor, m_penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(m_currentPath);
    }

    // draw live marquee selection rectangle
    if (m_drawingSelectionRect) {
        QPen selectionPen(SelectionLinesColour, SelectionLineWidth, SelectionLineType);
        selectionPen.setCosmetic(true);

        painter.setPen(selectionPen);
        painter.setBrush(SelectionRectangleColour);
        painter.drawRect(m_selectionRect.normalized());
    }

    // draw snapped border around selected items
    if (!m_selectedItems.empty()) {
        QPen selectionPen(SelectionLinesColour, SelectionLineWidth, SelectionLineType);
        selectionPen.setCosmetic(true);

        painter.setPen(selectionPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(selectedItemsBoundingRect());
    }
}

void CanvasWidget::mousePressEvent(QMouseEvent *event) {
    QPointF scenePos = screenToScene(event->position());
    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        m_lastPanScreenPos = event->position();
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (event->button() == Qt::LeftButton && m_mode == Mode::Pen) {
        m_selectedItems.clear();

        m_drawing = true;
        m_currentPath = QPainterPath();
        m_currentPath.moveTo(scenePos);

        update();
        return;
    }

    if (event->button() == Qt::LeftButton && m_mode == Mode::Erase) {
        eraseAt(scenePos);
        return;
    }

    if (event->button() == Qt::LeftButton && m_mode == Mode::Select) {
        QRectF currentSelectionBounds = selectedItemsBoundingRect();

        if (!m_selectedItems.empty() && currentSelectionBounds.contains(scenePos)) {
            // Move the existing group selection.
            m_draggingSelection = true;
            m_drawingSelectionRect = false;
            m_lastDragScenePos = scenePos;
        } else {
            // Start drawing a new translucent selection rectangle.
            m_selectedItems.clear();
            m_draggingSelection = false;
            m_drawingSelectionRect = true;
            m_selectionStartScenePos = scenePos;
            m_selectionRect = QRectF(scenePos, scenePos);
        }

        update();
        return;
    }
}

void CanvasWidget::mouseMoveEvent(QMouseEvent *event) {
    QPointF scenePos = screenToScene(event->position());

    if (m_panning) {
        QPointF delta = event->position() - m_lastPanScreenPos;
        m_offset += delta;
        m_lastPanScreenPos = event->position();
        update();
    }

    if (m_drawing && m_mode == Mode::Pen) {
        m_currentPath.lineTo(scenePos);
        update();
        return;
    }

    if (m_mode == Mode::Erase && event->buttons() & Qt::LeftButton) {
        eraseAt(scenePos);
        return;
    }

    if (m_mode == Mode::Select && m_draggingSelection && !m_selectedItems.empty()) {
        QPointF delta = scenePos - m_lastDragScenePos;

        for (CanvasItem *item : m_selectedItems) {
            item->moveBy(delta);
        }

        m_lastDragScenePos = scenePos;

        update();
        return;
    }

    if (m_mode == Mode::Select && m_drawingSelectionRect) {
        m_selectionRect = QRectF(m_selectionStartScenePos, scenePos).normalized();
        update();
        return;
    }
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton ||
        (event->button() == Qt::LeftButton && m_panning)) {
        m_panning = false;
        unsetCursor();
        return;
    }

    if (event->button() == Qt::LeftButton && m_drawing && m_mode == Mode::Pen) {
        m_drawing = false;

        auto stroke = std::make_unique<StrokeItem>(
            m_currentPath,
            m_penColor,
            m_penWidth
        );

        m_items.push_back(std::move(stroke));

        m_currentPath = QPainterPath();
        update();
        return;
    }

    if (event->button() == Qt::LeftButton && m_mode == Mode::Select) {
        if (m_drawingSelectionRect) {
            selectItemsInsideRect(m_selectionRect);
            m_drawingSelectionRect = false;
            m_selectionRect = QRectF();
        }

        m_draggingSelection = false;
        update();
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


void CanvasWidget::wheelEvent(QWheelEvent *event)
{
    QPointF mouseSceneBeforeZoom = screenToScene(event->position());

    if (event->angleDelta().y() > 0)
        m_zoom *= ZoomFactor;
    else
        m_zoom /= ZoomFactor;

    m_zoom = std::clamp(m_zoom, MinZoom, MaxZoom);

    QPointF mouseSceneAfterZoom = screenToScene(event->position());

    m_offset += (mouseSceneAfterZoom - mouseSceneBeforeZoom) * m_zoom;

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