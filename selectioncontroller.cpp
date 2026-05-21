#include "selectioncontroller.h"

namespace {
const QColor SelectionLinesColour = Qt::blue;
const Qt::PenStyle SelectionLineType = Qt::DashLine;
const int SelectionLineWidth = 1;
const QColor SelectionRectangleColour = QColor(0, 120, 215, 40);

constexpr qreal HandleScreenSize = 10.0;
constexpr qreal MinSelectionSize = 5.0;
constexpr QColor HitboxColour = QColor(255, 0, 0, 60);
}

SelectionController::SelectionController()
{
    initializeHitboxes();
}

const std::array<SelectionController::RoleData, SelectionController::Role::Count>&
SelectionController::roleData()
{
    static const std::array<RoleData, Role::Count> data = {{
        // Body
        RoleData {
            .resize = false,
            .xFactor = 0.5,
            .yFactor = 0.5,
            .changesLeft = false,
            .changesTop = false,
            .changesRight = false,
            .changesBottom = false,
            .cursorShape = Qt::SizeAllCursor
        },

        // TopLeft
        RoleData {
            .resize = true,
            .xFactor = 0.0,
            .yFactor = 0.0,
            .changesLeft = true,
            .changesTop = true,
            .changesRight = false,
            .changesBottom = false,
            .cursorShape = Qt::SizeFDiagCursor
        },

        // Top
        RoleData {
            .resize = true,
            .xFactor = 0.5,
            .yFactor = 0.0,
            .changesLeft = false,
            .changesTop = true,
            .changesRight = false,
            .changesBottom = false,
            .cursorShape = Qt::SizeVerCursor
        },

        // TopRight
        RoleData {
            .resize = true,
            .xFactor = 1.0,
            .yFactor = 0.0,
            .changesLeft = false,
            .changesTop = true,
            .changesRight = true,
            .changesBottom = false,
            .cursorShape = Qt::SizeBDiagCursor
        },

        // Right
        RoleData {
            .resize = true,
            .xFactor = 1.0,
            .yFactor = 0.5,
            .changesLeft = false,
            .changesTop = false,
            .changesRight = true,
            .changesBottom = false,
            .cursorShape = Qt::SizeHorCursor
        },

        // BottomRight
        RoleData {
            .resize = true,
            .xFactor = 1.0,
            .yFactor = 1.0,
            .changesLeft = false,
            .changesTop = false,
            .changesRight = true,
            .changesBottom = true,
            .cursorShape = Qt::SizeFDiagCursor
        },

        // Bottom
        RoleData {
            .resize = true,
            .xFactor = 0.5,
            .yFactor = 1.0,
            .changesLeft = false,
            .changesTop = false,
            .changesRight = false,
            .changesBottom = true,
            .cursorShape = Qt::SizeVerCursor
        },

        // BottomLeft
        RoleData {
            .resize = true,
            .xFactor = 0.0,
            .yFactor = 1.0,
            .changesLeft = true,
            .changesTop = false,
            .changesRight = false,
            .changesBottom = true,
            .cursorShape = Qt::SizeBDiagCursor
        },

        // Left
        RoleData {
            .resize = true,
            .xFactor = 0.0,
            .yFactor = 0.5,
            .changesLeft = true,
            .changesTop = false,
            .changesRight = false,
            .changesBottom = false,
            .cursorShape = Qt::SizeHorCursor
        }
    }};

    return data;
}

bool SelectionController::isValidRole(int role) const
{
    return role >= 0 && role < Role::Count;
}

bool SelectionController::isResizeRole(int role) const
{
    return isValidRole(role) && roleData()[role].resize;
}

void SelectionController::initializeHitboxes()
{
    for (int role = 0; role < Role::Count; ++role) {
        m_hitboxes[role].owner = this;
        m_hitboxes[role].role = role;
        m_hitboxes[role].rect = QRectF();
        m_hitboxes[role].cursorShape = roleData()[role].cursorShape;
    }
}

void SelectionController::setViewScale(qreal zoom) {
    if (zoom < 0)
    {
        m_viewScale = 1.0;
        return;
    }
    m_viewScale = zoom;
    updateHitboxes();
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

QPointF SelectionController::handleCenterForRole(Role role, const QRectF &bounds) const
{
    const RoleData &data = roleData()[role];

    return QPointF(
        bounds.left() + data.xFactor * bounds.width(),
        bounds.top() + data.yFactor * bounds.height()
        );
}

QRectF SelectionController::handleRectAt(const QPointF &center) const
{
    const qreal sceneSize = HandleScreenSize / m_viewScale;
    return QRectF(
        center.x() - sceneSize / 2.0,
        center.y() - sceneSize / 2.0,
        sceneSize,
        sceneSize
        );
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

    if (m_selectedItems.empty()) {
        return;
    }

    painter.setPen(selectionPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(boundingRect());

    QPen hitboxPen(Qt::red, 1, Qt::SolidLine);
    hitboxPen.setCosmetic(true);

    painter.setPen(hitboxPen);
    painter.setBrush(HitboxColour);

    for (int role = Role::TopLeft; role < Role::Count; ++role) {
        painter.drawRect(m_hitboxes[role].rect);
    }
}

std::vector<Hitbox*> SelectionController::hitboxes()
{
    if (m_selectedItems.empty()) {
        return {};
    }

    updateHitboxes();

    std::vector<Hitbox*> result;
    result.reserve(Role::Count);

    // Body first, handles after body.
    // CanvasWidget reverse-iterates hitboxes, so handles get priority over body.
    for (Hitbox &hitbox : m_hitboxes) {
        result.push_back(&hitbox);
    }

    return result;
}

void SelectionController::onHitboxPressed(int role, const QPointF &scenePos)
{
    if (!isValidRole(role)) {
        return;
    }

    m_lastDragScenePos = scenePos;

    if (isResizeRole(role)) {
        m_resizeStartBounds = boundingRect();
    }
}

void SelectionController::moveSelectedItems(const QPointF &scenePos)
{
    QPointF delta = scenePos - m_lastDragScenePos;

    for (CanvasItem *item : m_selectedItems) {
        item->moveBy(delta);
    }

    m_lastDragScenePos = scenePos;
    updateHitboxes();
}


void SelectionController::onHitboxDragged(int role, const QPointF &scenePos)
{
    if (!isValidRole(role)) {
        return;
    }

    if (role == Role::Body) {
        moveSelectedItems(scenePos);
        return;
    }

    if (isResizeRole(role)) {
        resizeSelectedItems(role, scenePos);
        return;
    }
}

void SelectionController::onHitboxReleased(int role, const QPointF &scenePos)
{
    Q_UNUSED(role);
    Q_UNUSED(scenePos);

    m_resizeStartBounds = QRectF();
    updateHitboxes();
}

void SelectionController::resizeSelectedItems(int role, const QPointF &scenePos)
{
    QRectF oldBounds = boundingRect();

    if (oldBounds.isNull() ||
        oldBounds.width() == 0.0 ||
        oldBounds.height() == 0.0) {
        return;
    }

    QRectF newBounds = oldBounds;

    const RoleData &data = roleData()[role];

    if (data.changesLeft) {
        newBounds.setLeft(scenePos.x());
    }

    if (data.changesTop) {
        newBounds.setTop(scenePos.y());
    }

    if (data.changesRight) {
        newBounds.setRight(scenePos.x());
    }

    if (data.changesBottom) {
        newBounds.setBottom(scenePos.y());
    }

    newBounds = newBounds.normalized();

    if (newBounds.width() < MinSelectionSize) {
        newBounds.setWidth(MinSelectionSize);
    }

    if (newBounds.height() < MinSelectionSize) {
        newBounds.setHeight(MinSelectionSize);
    }

    for (CanvasItem *item : m_selectedItems) {
        item->transformFromRect(oldBounds, newBounds);
    }

    updateHitboxes();
}


void SelectionController::updateHitboxes()
{
    for (Hitbox &hitbox : m_hitboxes) {
        hitbox.rect = QRectF();
    }

    if (m_selectedItems.empty()) {
        return;
    }

    QRectF bounds = boundingRect();

    m_hitboxes[Role::Body].rect = bounds;

    for (int role = Role::TopLeft; role < Role::Count; ++role) {
        QPointF center = handleCenterForRole(static_cast<Role>(role), bounds);
        m_hitboxes[role].rect = handleRectAt(center);
    }
}