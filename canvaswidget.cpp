#include "canvaswidget.h"

#include "imageitem.h"
#include "milegriditem.h"
#include "strokeitem.h"
#include "textboxitem.h"

#include <algorithm>
#include <cmath>
#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QTextEdit>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextBlock>
#include <QTextFragment>
#include <QWheelEvent>

namespace {
constexpr int DeltaXAfterCopy = 30;
constexpr int DeltaYAfterCopy = 30;

const QColor BackgroundColour = Qt::white;

const QColor GridColour = QColor(230,230,230);
constexpr int GridSize = 50;

const double ZoomFactor = 1.15;
const double MinZoom = 0.2;
const double MaxZoom = 5.0;

const QSizeF DefaultTextBoxSceneSize(260, 120);
constexpr int MinTextEditorScreenWidth = 80;
constexpr int MinTextEditorScreenHeight = 48;
constexpr qreal TextDocumentSceneMargin = 4.0;
constexpr qreal ZoomEpsilon = 0.0001;
constexpr qreal DefaultImageMaxWidth = 420.0;
constexpr qreal DefaultImageMaxHeight = 320.0;

bool fuzzyRectEquals(const QRectF &left, const QRectF &right)
{
    constexpr qreal Epsilon = 0.001;

    return std::abs(left.left() - right.left()) < Epsilon &&
           std::abs(left.top() - right.top()) < Epsilon &&
           std::abs(left.width() - right.width()) < Epsilon &&
           std::abs(left.height() - right.height()) < Epsilon;
}

}

CanvasWidget::CanvasWidget(QWidget *parent)
    : QWidget{parent}
{
    setAttribute(Qt::WA_StaticContents);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet("background-color: white;");

    registerHitboxOwner(&m_selection);
}

void CanvasWidget::setMode(Mode mode) {
    commitActiveTextEditor();
    clearPendingTextBoxCreation();

    m_mode = mode;
    m_selection.clear();
    m_activeHitbox.reset();
    resetCanvasCursor();
    update();
}

void CanvasWidget::setPenColor(const QColor &color) {
    m_penColor = color;
}

void CanvasWidget::setPenWidth(int width) {
    m_penWidth = width;
}

void CanvasWidget::deleteSelection() {
    commitActiveTextEditor();
    clearPendingTextBoxCreation();

    CanvasItemList before = cloneItems();

    clearActiveHitboxIfOwnedBy(&m_selection);
    m_selection.deleteSelectedItems(m_items);

    pushSnapshotCommand(std::move(before), cloneItems());
    update();
}

void CanvasWidget::duplicateSelection() {
    commitActiveTextEditor();
    clearPendingTextBoxCreation();

    CanvasItemList before = cloneItems();
    const std::size_t firstNewItemIndex = m_items.size();

    m_selection.duplicateSelectedItems(
        m_items,
        QPointF(DeltaXAfterCopy, DeltaYAfterCopy)
        );

    for (std::size_t i = firstNewItemIndex; i < m_items.size(); ++i) {
        assignFreshId(*m_items[i]);
    }

    pushSnapshotCommand(std::move(before), cloneItems());
    update();
}
QPointF CanvasWidget::screenToScene(const QPointF &screenPoint) const {
    return (screenPoint - m_offset) / m_zoom;
}

QPointF CanvasWidget::sceneToScreen(const QPointF &screenPoint) const {
    return screenPoint * m_zoom + m_offset;
}

QPointF CanvasWidget::visibleSceneCenter() const
{
    return screenToScene(QPointF(width() / 2.0, height() / 2.0));
}

QRectF CanvasWidget::defaultImageRect(const QImage &image) const
{
    if (image.isNull()) {
        return QRectF(visibleSceneCenter(), QSizeF(DefaultImageMaxWidth, DefaultImageMaxHeight));
    }

    QSizeF imageSize = image.size();
    imageSize.scale(
        QSizeF(DefaultImageMaxWidth, DefaultImageMaxHeight),
        Qt::KeepAspectRatio
        );

    const QPointF center = visibleSceneCenter();
    return QRectF(
        center.x() - imageSize.width() / 2.0,
        center.y() - imageSize.height() / 2.0,
        imageSize.width(),
        imageSize.height()
        );
}

bool CanvasWidget::insertImage(const QImage &image)
{
    if (image.isNull()) {
        return false;
    }

    commitActiveTextEditor();
    clearPendingTextBoxCreation();

    CanvasItemList before = cloneItems();

    auto item = std::make_unique<ImageItem>(defaultImageRect(image), image);
    assignFreshId(*item);
    m_items.push_back(std::move(item));

    pushSnapshotCommand(std::move(before), cloneItems());
    update();

    return true;
}

bool CanvasWidget::insertImageFromFile(const QString &filePath)
{
    QImage image(filePath);
    return insertImage(image);
}

bool CanvasWidget::pasteImageFromClipboard()
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard ? clipboard->mimeData() : nullptr;

    if (!mimeData || !mimeData->hasImage()) {
        return false;
    }

    return insertImage(qvariant_cast<QImage>(mimeData->imageData()));
}

bool CanvasWidget::insertMileGrid(int columns, int rows)
{
    if (columns <= 0 || rows <= 0) {
        return false;
    }

    commitActiveTextEditor();
    clearPendingTextBoxCreation();

    CanvasItemList before = cloneItems();

    auto item = std::make_unique<MileGridItem>(
        visibleSceneCenter(),
        columns,
        rows,
        m_mileGridCellColor
        );
    assignFreshId(*item);
    m_items.push_back(std::move(item));

    pushSnapshotCommand(std::move(before), cloneItems());
    update();

    return true;
}

void CanvasWidget::setMileGridCellColor(const QColor &color)
{
    if (!color.isValid()) {
        return;
    }

    m_mileGridCellColor = color;

    MileGridItem::setCurrentColour(color);
}

CanvasItem *CanvasWidget::itemAt(const QPointF &scenePos) const {
    if (m_items.empty())
        return nullptr;
    for (int i = m_items.size() - 1; i >= 0; i--)
        if (m_items[i]->contains(scenePos))
            return m_items[i].get();

    return nullptr;
}

void CanvasWidget::bringItemToTop(CanvasItem *item)
{
    if (!item || m_items.empty()) {
        return;
    }

    auto itemIt = std::find_if(
        m_items.begin(),
        m_items.end(),
        [item](const std::unique_ptr<CanvasItem> &candidate) {
            return candidate.get() == item;
        }
        );

    if (itemIt == m_items.end() || std::next(itemIt) == m_items.end()) {
        return;
    }

    std::unique_ptr<CanvasItem> movedItem = std::move(*itemIt);
    m_items.erase(itemIt);
    m_items.push_back(std::move(movedItem));
}

CanvasItem *CanvasWidget::itemById(CanvasItemId id) const
{
    if (id == InvalidCanvasItemId) {
        return nullptr;
    }

    for (const auto &item : m_items) {
        if (item && item->id() == id) {
            return item.get();
        }
    }

    return nullptr;
}

CanvasItemId CanvasWidget::allocateItemId()
{
    return m_nextItemId++;
}

void CanvasWidget::assignFreshId(CanvasItem &item)
{
    item.setId(allocateItemId());
}

CanvasItemList CanvasWidget::cloneItems() const
{
    CanvasItemList result;
    result.reserve(m_items.size());

    for (const auto &item : m_items) {
        if (item) {
            result.push_back(item->clone());
        }
    }

    return result;
}

bool CanvasWidget::snapshotsEquivalent(const CanvasItemList &left, const CanvasItemList &right) const
{
    if (left.size() != right.size()) {
        return false;
    }

    for (std::size_t i = 0; i < left.size(); ++i) {
        const CanvasItem *leftItem = left[i].get();
        const CanvasItem *rightItem = right[i].get();

        if (!leftItem || !rightItem) {
            return leftItem == rightItem;
        }

        if (leftItem->id() != rightItem->id()) {
            return false;
        }

        if (!fuzzyRectEquals(leftItem->boundingRect(), rightItem->boundingRect())) {
            return false;
        }
    }

    return true;
}

void CanvasWidget::executeCommand(std::unique_ptr<CanvasCommand> command)
{
    if (!command) {
        return;
    }

    command->redo(*this);
    m_undoStack.push_back(std::move(command));
    m_redoStack.clear();
    update();
}

void CanvasWidget::pushSnapshotCommand(CanvasItemList before, CanvasItemList after, bool force)
{
    if (!force && snapshotsEquivalent(before, after)) {
        return;
    }

    m_undoStack.push_back(std::make_unique<CanvasSnapshotCommand>(
        std::move(before),
        std::move(after)
        ));
    m_redoStack.clear();
}

void CanvasWidget::restoreItemsFromSnapshot(const CanvasItemList &snapshot)
{
    m_selection.clear();
    m_activeHitbox.reset();
    m_itemsBeforeHitboxDrag.clear();

    m_items.clear();
    m_items.reserve(snapshot.size());

    for (const auto &item : snapshot) {
        if (item) {
            m_items.push_back(item->clone());
        }
    }

    updateNextItemIdFromItems();
    update();
}

void CanvasWidget::updateNextItemIdFromItems()
{
    CanvasItemId maxId = InvalidCanvasItemId;

    for (const auto &item : m_items) {
        if (item) {
            maxId = std::max(maxId, item->id());
        }
    }

    m_nextItemId = maxId + 1;
}

void CanvasWidget::undo()
{
    commitActiveTextEditor();
    clearPendingTextBoxCreation();

    if (m_undoStack.empty()) {
        return;
    }

    std::unique_ptr<CanvasCommand> command = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    command->undo(*this);
    m_redoStack.push_back(std::move(command));
}

void CanvasWidget::redo()
{
    commitActiveTextEditor();
    clearPendingTextBoxCreation();

    if (m_redoStack.empty()) {
        return;
    }

    std::unique_ptr<CanvasCommand> command = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    command->redo(*this);
    m_undoStack.push_back(std::move(command));
}

void CanvasWidget::setActiveTextFontFamily(const QString &family)
{
    m_activeTextFontFamily = family;
    mergeActiveTextCharFormat(scaledToolbarTextFormat());
}

void CanvasWidget::setActiveTextFontPointSize(int pointSize)
{
    if (pointSize <= 0) {
        return;
    }

    m_activeTextPointSize = pointSize;
    mergeActiveTextCharFormat(scaledToolbarTextFormat());
}

void CanvasWidget::setActiveTextBold(bool bold)
{
    m_activeTextBold = bold;
    mergeActiveTextCharFormat(scaledToolbarTextFormat());
}

void CanvasWidget::setActiveTextItalic(bool italic)
{
    m_activeTextItalic = italic;
    mergeActiveTextCharFormat(scaledToolbarTextFormat());
}

void CanvasWidget::setActiveTextUnderline(bool underline)
{
    m_activeTextUnderline = underline;
    mergeActiveTextCharFormat(scaledToolbarTextFormat());
}

void CanvasWidget::setActiveTextColor(const QColor &color)
{
    if (!color.isValid()) {
        return;
    }

    m_activeTextColor = color;
    mergeActiveTextCharFormat(scaledToolbarTextFormat());
}

void CanvasWidget::registerHitboxOwner(HitboxOwner *owner)
{
    if (!owner) {
        return;
    }

    m_hitboxOwners.push_back(owner);
}

Hitbox *CanvasWidget::hitboxAtForOwner(HitboxOwner *owner, const QPointF &scenePos) const
{
    if (!owner) {
        return nullptr;
    }

    std::vector<Hitbox*> hitboxes = owner->hitboxes();

    for (auto hitboxIt = hitboxes.rbegin(); hitboxIt != hitboxes.rend(); ++hitboxIt) {
        Hitbox *hitbox = *hitboxIt;

        if (hitbox && hitbox->owner && hitbox->contains(scenePos)) {
            return hitbox;
        }
    }

    return nullptr;
}

Hitbox *CanvasWidget::hitboxAt(const QPointF &scenePos) const
{
    // 1. First check registered non-item owners, for example SelectionController.
    for (auto ownerIt = m_hitboxOwners.rbegin(); ownerIt != m_hitboxOwners.rend(); ++ownerIt) {
        HitboxOwner *owner = *ownerIt;

        if (Hitbox *hitbox = hitboxAtForOwner(owner, scenePos)) {
            return hitbox;
        }
    }

    // 2. Then check canvas items that are also HitboxOwner.
    // Reverse order because topmost/latest item should win.
    for (auto itemIt = m_items.rbegin(); itemIt != m_items.rend(); ++itemIt) {
        CanvasItem *item = itemIt->get();

        if (!item) {
            continue;
        }

        if (auto textBox = dynamic_cast<TextBoxItem*>(item)) {
            textBox->setViewScale(m_zoom);
        }

        if (auto imageItem = dynamic_cast<ImageItem*>(item)) {
            imageItem->setViewScale(m_zoom);
        }

        if (auto mileGrid = dynamic_cast<MileGridItem*>(item)) {
            mileGrid->setViewScale(m_zoom);
        }

        if (auto owner = dynamic_cast<HitboxOwner*>(item)) {
            if (Hitbox *hitbox = hitboxAtForOwner(owner, scenePos)) {
                return hitbox;
            }
        }
    }

    return nullptr;
}
void CanvasWidget::clearActiveHitboxIfOwnedBy(HitboxOwner *owner)
{
    if (!m_activeHitbox.has_value()) {
        return;
    }

    if (m_activeHitbox->owner == owner) {
        m_activeHitbox.reset();
    }
}

void CanvasWidget::handleHitboxPress(Hitbox *hitbox, const QPointF &scenePos)
{
    if (!hitbox || !hitbox->owner) {
        return;
    }

    m_activeHitbox = ActiveHitbox {
        .owner = hitbox->owner,
        .role = hitbox->role,
        .cursorShape = hitbox->cursorShape
    };
    m_itemsBeforeHitboxDrag = cloneItems();

    if (auto *item = dynamic_cast<CanvasItem*>(hitbox->owner)) {
        bringItemToTop(item);
        m_selection.selectSingleItem(item);
    }

    setCursor(hitbox->cursor());

    hitbox->owner->onHitboxPressed(hitbox->role, scenePos);
    update();
}

bool CanvasWidget::handleActiveHitboxDrag(const QPointF &scenePos)
{
    if (!m_activeHitbox.has_value()) {
        return false;
    }

    ActiveHitbox &active = *m_activeHitbox;

    if (!active.owner) {
        m_activeHitbox.reset();
        return false;
    }

    active.owner->onHitboxDragged(active.role, scenePos);
    update();

    return true;
}

bool CanvasWidget::handleActiveHitboxRelease(const QPointF &scenePos)
{
    if (!m_activeHitbox.has_value()) {
        return false;
    }

    ActiveHitbox active = *m_activeHitbox;

    if (active.owner) {
        active.owner->onHitboxReleased(active.role, scenePos);
    }

    pushSnapshotCommand(std::move(m_itemsBeforeHitboxDrag), cloneItems());
    m_itemsBeforeHitboxDrag.clear();
    m_activeHitbox.reset();

    updateCursorForPosition(scenePos);
    update();

    return true;
}

bool CanvasWidget::handleMileGridTogglePress(CanvasItem *item, const QPointF &scenePos)
{
    auto *mileGrid = dynamic_cast<MileGridItem*>(item);

    if (!mileGrid || !mileGrid->containsGridPoint(scenePos)) {
        return false;
    }

    CanvasItemList before = cloneItems();

    m_selection.clear();
    m_activeHitbox.reset();
    bringItemToTop(mileGrid);

    if (!mileGrid->toggleCellAt(scenePos)) {
        return false;
    }

    pushSnapshotCommand(std::move(before), cloneItems());
    update();

    return true;
}

void CanvasWidget::updateCursorForPosition(const QPointF &scenePos)
{
    if (m_panning || m_activeHitbox.has_value()) {
        return;
    }

    if (m_mode == Mode::Select) {
        if (Hitbox *hitbox = hitboxAt(scenePos)) {
            setCursor(hitbox->cursor());
            return;
        }
    }

    resetCanvasCursor();
}

void CanvasWidget::resetCanvasCursor()
{
    switch (m_mode) {
    case Mode::Pen:
        setCursor(Qt::CrossCursor);
        return;

    case Mode::Erase:
        setCursor(Qt::PointingHandCursor);
        return;

    case Mode::Select:
        setCursor(Qt::ArrowCursor);
        return;
    }
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
        if (auto textBox = dynamic_cast<TextBoxItem*>(item.get())) {
            textBox->setViewScale(m_zoom);

            if (item && item->id() == m_editingTextItemId) {
                textBox->paintChrome(painter);
                continue;
            }
        }

        if (auto imageItem = dynamic_cast<ImageItem*>(item.get())) {
            imageItem->setViewScale(m_zoom);
        }

        if (auto mileGrid = dynamic_cast<MileGridItem*>(item.get())) {
            mileGrid->setViewScale(m_zoom);
        }

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

void CanvasWidget::drawSelection(QPainter &painter)
{
    m_selection.setViewScale(m_zoom);
    m_selection.paint(painter);
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
    drawSelection(painter);
}

void CanvasWidget::startPanning(const QPointF &screenPos) {
    m_panning = true;
    m_lastPanScreenPos = screenPos;
    setCursor(Qt::ClosedHandCursor);
}

void CanvasWidget::startPenStroke(const QPointF &scenePos) {
    m_selection.clear();
    m_activeHitbox.reset();

    m_drawing = true;
    m_currentPath = QPainterPath();
    m_currentPath.moveTo(scenePos);

    update();
}

void CanvasWidget::handleSelectPress(const QPointF &scenePos)
{
    if (Hitbox *selectionHitbox = hitboxAtForOwner(&m_selection, scenePos)) {
        handleHitboxPress(selectionHitbox, scenePos);
        return;
    }

    if (CanvasItem *item = itemAt(scenePos)) {
        if (auto *mileGrid = dynamic_cast<MileGridItem*>(item)) {
            mileGrid->setViewScale(m_zoom);

            if (mileGrid->containsGridPoint(scenePos) &&
                handleMileGridTogglePress(mileGrid, scenePos)) {
                return;
            }
        }
    }

    if (Hitbox *hitbox = hitboxAt(scenePos)) {
        handleHitboxPress(hitbox, scenePos);
        return;
    }

    if (CanvasItem *item = itemAt(scenePos)) {
        bringItemToTop(item);
        m_selection.selectSingleItem(item);
        update();
        return;
    }

    m_selection.beginSelectionRectangle(scenePos);
    update();
}

void CanvasWidget::clearActiveHitboxIfOwnedByItem(CanvasItem *item)
{
    if (!item) {
        return;
    }

    if (auto owner = dynamic_cast<HitboxOwner*>(item)) {
        clearActiveHitboxIfOwnedBy(owner);
    }
}

void CanvasWidget::eraseAt(const QPointF &scenePos) {
    if (m_items.empty()) {
        return;
    }

    for (int i = static_cast<int>(m_items.size()) - 1; i >= 0; --i) {
        if (m_items[i]->contains(scenePos)) {
            CanvasItemList before = cloneItems();

            m_selection.clear();
            m_activeHitbox.reset();

            clearActiveHitboxIfOwnedByItem(m_items[i].get());
            m_items.erase(m_items.begin() + i);
            pushSnapshotCommand(std::move(before), cloneItems());
            update();
            return;
        }
    }
}

void CanvasWidget::mousePressEvent(QMouseEvent *event) {
    if (m_activeTextEditor &&
        !m_activeTextEditor->geometry().contains(event->position().toPoint())) {
        commitActiveTextEditor();
    }

    const QPointF scenePos = screenToScene(event->position());

    if (event->button() == Qt::MiddleButton) {
        clearPendingTextBoxCreation();
        startPanning(event->position());
        return;
    }

    if (event->button() != Qt::LeftButton) {
        return;
    }

    clearPendingTextBoxCreation();

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
    updateActiveTextEditorGeometry();
    update();
}

void CanvasWidget::continuePenStroke(const QPointF &scenePos) {
    m_currentPath.lineTo(scenePos);
    update();
}

void CanvasWidget::continueErasing(const QPointF &scenePos) {
    eraseAt(scenePos);
}

void CanvasWidget::mouseMoveEvent(QMouseEvent *event) {
    const QPointF scenePos = screenToScene(event->position());

    updateCursorForPosition(scenePos);

    if (m_panning) {
        continuePanning(event->position());
        return;
    }

    if (handleActiveHitboxDrag(scenePos)) {
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
        if (m_selection.isDrawingSelectionRectangle()) {
            m_selection.updateSelectionRectangle(scenePos);
            update();
        }
        return;
    }
}

void CanvasWidget::finishPanning() {
    m_panning = false;
    resetCanvasCursor();
}

void CanvasWidget::finishPenStroke() {
    m_drawing = false;

    CanvasItemList before = cloneItems();

    auto stroke = std::make_unique<StrokeItem>(
        m_currentPath,
        m_penColor,
        m_penWidth
        );
    assignFreshId(*stroke);

    m_items.push_back(std::move(stroke));

    pushSnapshotCommand(std::move(before), cloneItems());
    m_currentPath = QPainterPath();
    update();
}

void CanvasWidget::finishSelection() {
    m_selection.finishSelectionRectangle(m_items);
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

    const QPointF scenePos = screenToScene(event->position());

    if (handleActiveHitboxRelease(scenePos)) {
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

void CanvasWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Paste) && pasteImageFromClipboard()) {
        event->accept();
        return;
    }

    if (m_pendingTextBoxScenePos.has_value()) {
        if (event->key() == Qt::Key_Escape) {
            clearPendingTextBoxCreation();
            event->accept();
            return;
        }

        if (shouldCreatePendingTextBoxFromKey(event)) {
            const QPointF scenePos = *m_pendingTextBoxScenePos;
            clearPendingTextBoxCreation();
            beginCreatingTextBox(scenePos);

            if (m_activeTextEditor) {
                QApplication::sendEvent(m_activeTextEditor, event);
                event->accept();
                return;
            }
        }
    }

    QWidget::keyPressEvent(event);
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

    updateActiveTextEditorGeometry();
    update();
}

void CanvasWidget::wheelEvent(QWheelEvent *event)
{
    handleZoom(event->position(), event->angleDelta().y());
}

bool CanvasWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_activeTextEditor) {
        if (event->type() == QEvent::KeyPress) {
            auto *keyEvent = static_cast<QKeyEvent*>(event);

            if (keyEvent->key() == Qt::Key_Escape) {
                cancelActiveTextEditor();
                return true;
            }

            if ((keyEvent->modifiers() & Qt::ControlModifier) &&
                (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)) {
                commitActiveTextEditor();
                return true;
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void CanvasWidget::createTextEditorAt(const QPointF &scenePos)
{
    commitActiveTextEditor();

    if (auto *textBox = dynamic_cast<TextBoxItem*>(itemAt(scenePos))) {
        clearPendingTextBoxCreation();
        beginEditingTextBox(textBox);
        return;
    }

    armTextBoxCreation(scenePos);
}

void CanvasWidget::armTextBoxCreation(const QPointF &scenePos)
{
    m_pendingTextBoxScenePos = scenePos;
    setFocus();
}

void CanvasWidget::clearPendingTextBoxCreation()
{
    m_pendingTextBoxScenePos.reset();
}

bool CanvasWidget::shouldCreatePendingTextBoxFromKey(const QKeyEvent *event) const
{
    if (!event) {
        return false;
    }

    const Qt::KeyboardModifiers modifiers = event->modifiers();
    if (modifiers.testFlag(Qt::ControlModifier) ||
        modifiers.testFlag(Qt::AltModifier) ||
        modifiers.testFlag(Qt::MetaModifier)) {
        return false;
    }

    const QString text = event->text();
    for (const QChar character : text) {
        if (character.isPrint()) {
            return true;
        }
    }

    return false;
}

void CanvasWidget::beginEditingTextBox(TextBoxItem *item)
{
    if (!item) {
        return;
    }

    m_editingNewTextItem = false;
    m_editingTextItemId = item->id();
    m_activeTextEditorSceneRect = item->rect();
    m_textBeforeEdit = item->html();
    m_itemsBeforeTextEdit = cloneItems();
    m_hasTextEditSnapshot = true;
    m_textEditorAppliedZoom = 1.0;

    auto *editor = new QTextEdit(this);
    m_activeTextEditor = editor;
    editor->setAcceptRichText(true);
    editor->setTextColor(Qt::black);
    editor->setHtml(item->html());
    editor->setFrameStyle(QFrame::NoFrame);
    editor->setContentsMargins(0, 0, 0, 0);
    editor->setAutoFillBackground(false);
    editor->viewport()->setAutoFillBackground(false);
    editor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    editor->setLineWrapMode(QTextEdit::WidgetWidth);
    editor->setStyleSheet("QTextEdit { background: transparent; color: black; border: none; padding: 0px; }");
    configureActiveTextEditorAppearance();
    mergeActiveTextCharFormat(scaledToolbarTextFormat());
    editor->installEventFilter(this);
    updateActiveTextEditorGeometry();
    editor->show();
    editor->setFocus();
    editor->moveCursor(QTextCursor::End);
    update();
}

void CanvasWidget::beginCreatingTextBox(const QPointF &scenePos)
{
    m_itemsBeforeTextEdit = cloneItems();
    m_hasTextEditSnapshot = true;

    auto item = std::make_unique<TextBoxItem>(scenePos, QString());
    assignFreshId(*item);

    TextBoxItem *newTextBox = item.get();
    m_items.push_back(std::move(item));

    m_editingNewTextItem = true;
    m_editingTextItemId = newTextBox->id();
    m_activeTextEditorSceneRect = newTextBox->rect();
    m_textBeforeEdit.clear();
    m_textEditorAppliedZoom = 1.0;

    auto *editor = new QTextEdit(this);
    m_activeTextEditor = editor;
    editor->setAcceptRichText(true);
    editor->setTextColor(Qt::black);
    editor->setFrameStyle(QFrame::NoFrame);
    editor->setContentsMargins(0, 0, 0, 0);
    editor->setAutoFillBackground(false);
    editor->viewport()->setAutoFillBackground(false);
    editor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    editor->setLineWrapMode(QTextEdit::WidgetWidth);
    editor->setStyleSheet("QTextEdit { background: transparent; color: black; border: none; padding: 0px; }");
    configureActiveTextEditorAppearance();
    mergeActiveTextCharFormat(scaledToolbarTextFormat());
    editor->installEventFilter(this);
    updateActiveTextEditorGeometry();
    editor->show();
    editor->setFocus();
    update();
}

void CanvasWidget::commitActiveTextEditor()
{
    clearPendingTextBoxCreation();

    if (!m_activeTextEditor) {
        return;
    }

    CanvasItemList before = m_hasTextEditSnapshot ? std::move(m_itemsBeforeTextEdit) : cloneItems();
    m_committingTextEdit = true;

    QTextEdit *editor = m_activeTextEditor;
    const QString html = normalizedHtmlFromEditor();
    const QString plainText = editor->toPlainText().trimmed();
    bool shouldForceSnapshot = false;

    if (m_editingNewTextItem) {
        if (TextBoxItem *item = activeTextBoxItem()) {
            if (plainText.isEmpty()) {
                clearActiveHitboxIfOwnedByItem(item);
                std::erase_if(m_items, [item](const std::unique_ptr<CanvasItem> &candidate) {
                    return candidate.get() == item;
                });
            } else {
                item->setRect(m_activeTextEditorSceneRect);
                item->setHtml(html);
            }
        }
    } else if (TextBoxItem *item = activeTextBoxItem()) {
        shouldForceSnapshot = item->html() != html;
        item->setRect(m_activeTextEditorSceneRect);
        item->setHtml(html);
    }

    pushSnapshotCommand(std::move(before), cloneItems(), shouldForceSnapshot);

    editor->removeEventFilter(this);
    editor->deleteLater();
    m_activeTextEditor.clear();
    m_editingTextItemId = InvalidCanvasItemId;
    m_activeTextEditorSceneRect = QRectF();
    m_textBeforeEdit.clear();
    m_itemsBeforeTextEdit.clear();
    m_hasTextEditSnapshot = false;
    m_editingNewTextItem = false;
    m_committingTextEdit = false;
    m_textEditorAppliedZoom = 1.0;

    setFocus();
    update();
}

void CanvasWidget::cancelActiveTextEditor()
{
    clearPendingTextBoxCreation();

    if (!m_activeTextEditor) {
        return;
    }

    QTextEdit *editor = m_activeTextEditor;
    editor->removeEventFilter(this);
    editor->deleteLater();

    if (m_editingNewTextItem && m_hasTextEditSnapshot) {
        restoreItemsFromSnapshot(m_itemsBeforeTextEdit);
    }

    m_activeTextEditor.clear();
    m_editingTextItemId = InvalidCanvasItemId;
    m_activeTextEditorSceneRect = QRectF();
    m_textBeforeEdit.clear();
    m_itemsBeforeTextEdit.clear();
    m_hasTextEditSnapshot = false;
    m_editingNewTextItem = false;
    m_committingTextEdit = false;
    m_textEditorAppliedZoom = 1.0;

    setFocus();
    update();
}

void CanvasWidget::configureActiveTextEditorAppearance()
{
    if (!m_activeTextEditor) {
        return;
    }

    QFont font = TextBoxItem::textFont();
    const qreal pointSize = font.pointSizeF() > 0.0
                                ? font.pointSizeF()
                                : static_cast<qreal>(font.pointSize());

    const qreal scaleFactor = m_zoom / m_textEditorAppliedZoom;
    if (std::abs(scaleFactor - 1.0) > ZoomEpsilon) {
        scaleDocumentFontSizes(*m_activeTextEditor->document(), scaleFactor);
        m_textEditorAppliedZoom = m_zoom;
    }

    font.setPointSizeF(pointSize * m_textEditorAppliedZoom);
    m_activeTextEditor->setFont(font);
    m_activeTextEditor->document()->setDefaultFont(font);
    m_activeTextEditor->document()->setDocumentMargin(TextDocumentSceneMargin * m_zoom);
    m_activeTextEditor->mergeCurrentCharFormat(scaledToolbarTextFormat());
}

QTextCharFormat CanvasWidget::currentToolbarTextFormat() const
{
    QTextCharFormat format;

    if (!m_activeTextFontFamily.isEmpty()) {
        format.setFontFamilies(QStringList{m_activeTextFontFamily});
    }

    format.setFontPointSize(m_activeTextPointSize);
    format.setFontWeight(m_activeTextBold ? QFont::Bold : QFont::Normal);
    format.setFontItalic(m_activeTextItalic);
    format.setFontUnderline(m_activeTextUnderline);
    format.setForeground(m_activeTextColor);

    return format;
}

QTextCharFormat CanvasWidget::scaledToolbarTextFormat() const
{
    QTextCharFormat format = currentToolbarTextFormat();
    format.setFontPointSize(m_activeTextPointSize * m_textEditorAppliedZoom);
    return format;
}

QString CanvasWidget::normalizedHtmlFromEditor() const
{
    if (!m_activeTextEditor) {
        return {};
    }

    QTextDocument document;
    document.setHtml(m_activeTextEditor->toHtml());

    QFont font = TextBoxItem::textFont();
    const qreal pointSize = font.pointSizeF() > 0.0
                                ? font.pointSizeF()
                                : static_cast<qreal>(font.pointSize());

    font.setPointSizeF(pointSize);
    document.setDefaultFont(font);
    document.setDocumentMargin(TextDocumentSceneMargin);
    if (m_textEditorAppliedZoom > ZoomEpsilon) {
        scaleDocumentFontSizes(document, 1.0 / m_textEditorAppliedZoom);
    }

    return document.toHtml();
}

void CanvasWidget::mergeActiveTextCharFormat(const QTextCharFormat &format)
{
    if (!m_activeTextEditor) {
        return;
    }

    QTextCursor cursor = m_activeTextEditor->textCursor();
    cursor.mergeCharFormat(format);
    m_activeTextEditor->mergeCurrentCharFormat(format);
    m_activeTextEditor->setTextCursor(cursor);
}

void CanvasWidget::scaleDocumentFontSizes(QTextDocument &document, qreal scaleFactor) const
{
    if (std::abs(scaleFactor - 1.0) <= ZoomEpsilon) {
        return;
    }

    for (QTextBlock block = document.begin(); block != document.end(); block = block.next()) {
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();

            if (!fragment.isValid()) {
                continue;
            }

            QTextCharFormat format = fragment.charFormat();
            qreal pointSize = format.fontPointSize();

            if (pointSize <= 0.0) {
                QFont defaultFont = document.defaultFont();
                pointSize = defaultFont.pointSizeF() > 0.0
                                ? defaultFont.pointSizeF()
                                : static_cast<qreal>(defaultFont.pointSize());
            }

            if (pointSize <= 0.0) {
                pointSize = TextBoxItem::textFont().pointSizeF();
            }

            format.setFontPointSize(pointSize * scaleFactor);

            QTextCursor cursor(&document);
            cursor.setPosition(fragment.position());
            cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
            cursor.mergeCharFormat(format);
        }
    }
}

void CanvasWidget::updateActiveTextEditorGeometry()
{
    if (!m_activeTextEditor) {
        return;
    }

    configureActiveTextEditorAppearance();

    const QPointF topLeft = sceneToScreen(m_activeTextEditorSceneRect.topLeft());
    const QSizeF scaledSize = m_activeTextEditorSceneRect.size() * m_zoom;
    const QSize screenSize(
        std::max(MinTextEditorScreenWidth, static_cast<int>(scaledSize.width())),
        std::max(MinTextEditorScreenHeight, static_cast<int>(scaledSize.height()))
        );

    m_activeTextEditor->setGeometry(QRect(topLeft.toPoint(), screenSize));
}

TextBoxItem *CanvasWidget::activeTextBoxItem() const
{
    return dynamic_cast<TextBoxItem*>(itemById(m_editingTextItemId));
}
