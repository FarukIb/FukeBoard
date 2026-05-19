#include "selectioncontroller.h"

namespace {
const QColor SelectionLinesColour = Qt::blue;
const Qt::PenStyle SelectionLineType = Qt::DashLine;
const int SelectionLineWidth = 1;
const QColor SelectionRectangleColour = QColor(0, 120, 215, 40);
}

SelectionController::SelectionController()
{
    m_bodyHitbox.owner = this;
    m_bodyHitbox.role = static_cast<int>(Role::Body);
    m_bodyHitbox.rect = QRectF();
}

void SelectionController::clear()
{
    m_selectedItems.clear();
    m_drawingSelectionRectangle = false;
    m_selectionRect = QRectF();
    updateHitboxes();
}

bool SelectionController::empty() const
{
    return m_selectedItems.empty();
}


const std::set<CanvasItem*> &SelectionController::selectedItems() const
{
    return m_selectedItems;
}

void SelectionController::beginSelectionRectangle(const QPointF &scenePos)
{
    clear();

    m_drawingSelectionRectangle = true;
    m_selectionStartScenePos = scenePos;
    m_selectionRect = QRectF(scenePos, scenePos);
}

void SelectionController::updateSelectionRectangle(const QPointF &scenePos)
{
    if (!m_drawingSelectionRectangle) {
        return;
    }

    m_selectionRect = QRectF(m_selectionStartScenePos, scenePos).normalized();
}


void SelectionController::finishSelectionRectangle(
    const std::vector<std::unique_ptr<CanvasItem>> &items
    )
{
    if (!m_drawingSelectionRectangle) {
        return;
    }

    selectItemsInsideRect(m_selectionRect, items);

    m_drawingSelectionRectangle = false;
    m_selectionRect = QRectF();

    updateHitboxes();
}

bool SelectionController::isDrawingSelectionRectangle() const
{
    return m_drawingSelectionRectangle;
}


void SelectionController::selectItemsInsideRect(
    const QRectF &rect,
    const std::vector<std::unique_ptr<CanvasItem>> &items
    )
{
    m_selectedItems.clear();

    QRectF normalizedRect = rect.normalized();

    for (const auto &item : items) {
        // Intersects feels better for strokes.
        if (normalizedRect.intersects(item->boundingRect())) {
            m_selectedItems.insert(item.get());
        }
    }

    updateHitboxes();
}

void SelectionController::deleteSelectedItems(
    std::vector<std::unique_ptr<CanvasItem>> &items
    )
{
    if (m_selectedItems.empty()) {
        return;
    }

    for (auto it = items.begin(); it != items.end(); ) {
        if (m_selectedItems.count(it->get())) {
            it = items.erase(it);
        } else {
            ++it;
        }
    }

    clear();
}

void SelectionController::duplicateSelectedItems(
    std::vector<std::unique_ptr<CanvasItem>> &items,
    const QPointF &copyOffset
    )
{
    if (m_selectedItems.empty()) {
        return;
    }

    std::set<CanvasItem*> copiedItems;

    for (CanvasItem *item : m_selectedItems) {
        auto newItem = item->clone();
        newItem->moveBy(copyOffset);

        copiedItems.insert(newItem.get());
        items.push_back(std::move(newItem));
    }

    m_selectedItems = copiedItems;
    updateHitboxes();
}

QRectF SelectionController::boundingRect() const
{
    if (m_selectedItems.empty()) {
        return QRectF();
    }

    QRectF result = (*m_selectedItems.begin())->boundingRect();

    for (CanvasItem *item : m_selectedItems) {
        result = result.united(item->boundingRect());
    }

    return result;
}


void SelectionController::paint(QPainter &painter) const
{
    QPen selectionPen(SelectionLinesColour, SelectionLineWidth, SelectionLineType);
    selectionPen.setCosmetic(true);

    if (m_drawingSelectionRectangle) {
        painter.setPen(selectionPen);
        painter.setBrush(SelectionRectangleColour);
        painter.drawRect(m_selectionRect.normalized());
    }

    if (!m_selectedItems.empty()) {
        painter.setPen(selectionPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(boundingRect());
    }
}


std::vector<Hitbox*> SelectionController::hitboxes()
{
    if (m_selectedItems.empty()) {
        return {};
    }

    updateHitboxes();

    return {
        &m_bodyHitbox
    };
}

void SelectionController::onHitboxPressed(int role, const QPointF &scenePos)
{
    if (role != static_cast<int>(Role::Body)) {
        return;
    }

    m_lastDragScenePos = scenePos;
}

void SelectionController::onHitboxDragged(int role, const QPointF &scenePos)
{
    if (role != static_cast<int>(Role::Body)) {
        return;
    }

    QPointF delta = scenePos - m_lastDragScenePos;

    for (CanvasItem *item : m_selectedItems) {
        item->moveBy(delta);
    }

    m_lastDragScenePos = scenePos;
    updateHitboxes();
}

void SelectionController::onHitboxReleased(int role, const QPointF &scenePos)
{
    Q_UNUSED(role);
    Q_UNUSED(scenePos);

    updateHitboxes();
}

void SelectionController::updateHitboxes()
{
    if (m_selectedItems.empty()) {
        m_bodyHitbox.rect = QRectF();
        return;
    }

    m_bodyHitbox.rect = boundingRect();
}