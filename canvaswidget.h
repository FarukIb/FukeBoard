#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include <QColor>
#include <QImage>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPointF>
#include <QPointer>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QWheelEvent>
#include <QWidget>

#include <memory>
#include <optional>
#include <vector>

#include "appconstants.h"
#include "canvascommand.h"
#include "canvasitem.h"
#include "hitbox.h"
#include "selectioncontroller.h"

class QTextEdit;
class TextBoxItem;

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

    void undo();
    void redo();
    void restoreItemsFromSnapshot(const CanvasItemList &snapshot);
    bool insertImageFromFile(const QString &filePath);
    bool pasteImageFromClipboard();
    bool insertMileGrid(int columns, int rows);
    void setMileGridCellColor(const QColor &color);

    void setActiveTextFontFamily(const QString &family);
    void setActiveTextFontPointSize(int pointSize);
    void setActiveTextBold(bool bold);
    void setActiveTextItalic(bool italic);
    void setActiveTextUnderline(bool underline);
    void setActiveTextColor(const QColor &color);
protected:
    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *EVENT) override;
    void keyPressEvent(QKeyEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    int m_penWidth = AppConstants::DefaultPenWidth;
    Mode m_mode = Mode::Pen;

    std::vector<HitboxOwner*> m_hitboxOwners;
    struct ActiveHitbox {
        HitboxOwner *owner = nullptr;
        int role = 0;
        Qt::CursorShape cursorShape = Qt::ArrowCursor;
    };

    void clearActiveHitboxIfOwnedByItem(CanvasItem *item);

    std::optional<ActiveHitbox> m_activeHitbox;

    std::vector<std::unique_ptr<CanvasItem>> m_items;
    CanvasItemId m_nextItemId = 1;
    std::vector<std::unique_ptr<CanvasCommand>> m_undoStack;
    std::vector<std::unique_ptr<CanvasCommand>> m_redoStack;
    CanvasItemList m_itemsBeforeHitboxDrag;

    QPointer<QTextEdit> m_activeTextEditor;
    std::optional<QPointF> m_pendingTextBoxScenePos;
    CanvasItemId m_editingTextItemId = InvalidCanvasItemId;
    QRectF m_activeTextEditorSceneRect;
    QString m_textBeforeEdit;
    CanvasItemList m_itemsBeforeTextEdit;
    bool m_hasTextEditSnapshot = false;
    bool m_editingNewTextItem = false;
    bool m_committingTextEdit = false;
    qreal m_textEditorAppliedZoom = 1.0;
    QString m_activeTextFontFamily;
    int m_activeTextPointSize = 14;
    bool m_activeTextBold = false;
    bool m_activeTextItalic = false;
    bool m_activeTextUnderline = false;
    QColor m_activeTextColor = Qt::black;
    QColor m_mileGridCellColor = QColor(20, 20, 20);

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
    QPointF visibleSceneCenter() const;
    QRectF defaultImageRect(const QImage &image) const;
    bool insertImage(const QImage &image);

    CanvasItem* itemAt(const QPointF &scenePos) const;
    CanvasItem* itemById(CanvasItemId id) const;
    void bringItemToTop(CanvasItem *item);
    CanvasItemId allocateItemId();
    void assignFreshId(CanvasItem &item);
    CanvasItemList cloneItems() const;
    bool snapshotsEquivalent(const CanvasItemList &left, const CanvasItemList &right) const;
    void executeCommand(std::unique_ptr<CanvasCommand> command);
    void pushSnapshotCommand(CanvasItemList before, CanvasItemList after, bool force = false);
    void updateNextItemIdFromItems();

    Hitbox *hitboxAtForOwner(HitboxOwner *owner, const QPointF &scenePos) const;
    Hitbox *hitboxAt(const QPointF &scenePos) const;
    void registerHitboxOwner(HitboxOwner *owner);
    void clearActiveHitboxIfOwnedBy(HitboxOwner *owner);

    void handleHitboxPress(Hitbox *hitbox, const QPointF &scenePos);
    bool handleActiveHitboxDrag(const QPointF &scenePos);
    bool handleActiveHitboxRelease(const QPointF &scenePos);
    bool handleMileGridTogglePress(CanvasItem *item, const QPointF &scenePos);
    void updateCursorForPosition(const QPointF &scenePos);
    void resetCanvasCursor();

    void eraseAt(const QPointF &scenePos);
    void createTextEditorAt(const QPointF &scenePos);
    void armTextBoxCreation(const QPointF &scenePos);
    void clearPendingTextBoxCreation();
    bool shouldCreatePendingTextBoxFromKey(const QKeyEvent *event) const;
    void beginEditingTextBox(TextBoxItem *item);
    void beginCreatingTextBox(const QPointF &scenePos);
    void commitActiveTextEditor();
    void cancelActiveTextEditor();
    void configureActiveTextEditorAppearance();
    QTextCharFormat currentToolbarTextFormat() const;
    QTextCharFormat scaledToolbarTextFormat() const;
    QString normalizedHtmlFromEditor() const;
    void mergeActiveTextCharFormat(const QTextCharFormat &format);
    void scaleDocumentFontSizes(QTextDocument &document, qreal scaleFactor) const;
    void updateActiveTextEditorGeometry();
    TextBoxItem *activeTextBoxItem() const;

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
