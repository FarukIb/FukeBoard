#include <QtTest>

#include <QClipboard>
#include <QImage>
#include <QTemporaryDir>

#include <set>

#include "canvasfile.h"
#include "canvaswidget.h"
#include "imageitem.h"
#include "strokeitem.h"

class CanvasWidgetTest : public QObject
{
    Q_OBJECT

private slots:
    void clickCreatesSelectableDot();
    void deleteKeyDeletesTheCurrentSelection();
    void copyAndPasteSelectionAssignsFreshIds();
    void copyAndPasteMultipleSelectedItems();
    void copyAndPasteImageItemPreservesPixels();
    void externalClipboardImageInvalidatesSelectionCopy();
};

void CanvasWidgetTest::clickCreatesSelectableDot()
{
    CanvasWidget canvas;
    canvas.resize(400, 300);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));

    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString filePath = temporaryDirectory.filePath("board.fuke");
    QVERIFY(canvas.saveFukeFile(filePath));

    CanvasItemList items;
    QVERIFY(CanvasFile::load(filePath, &items));
    QCOMPARE(items.size(), std::size_t(1));
    QVERIFY(dynamic_cast<StrokeItem*>(items.front().get()));

    canvas.setMode(Mode::Select);
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
    canvas.setFocus();
    QTest::keyClick(&canvas, Qt::Key_Delete);
    QVERIFY(canvas.saveFukeFile(filePath));
    QVERIFY(CanvasFile::load(filePath, &items));
    QCOMPARE(items.size(), std::size_t(0));
}

void CanvasWidgetTest::deleteKeyDeletesTheCurrentSelection()
{
    CanvasWidget canvas;
    QSignalSpy documentChangedSpy(&canvas, &CanvasWidget::documentChanged);
    canvas.resize(400, 300);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    QTest::mousePress(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));
    QTest::mouseMove(&canvas, QPoint(120, 50));
    QTest::mouseRelease(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(120, 50));

    canvas.setMode(Mode::Select);
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(80, 50));
    canvas.setFocus();
    QTest::keyClick(&canvas, Qt::Key_Delete);
    QCOMPARE(documentChangedSpy.count(), 2);

    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString filePath = temporaryDirectory.filePath("board.fuke");
    QVERIFY(canvas.saveFukeFile(filePath));

    CanvasItemList items;
    QVERIFY(CanvasFile::load(filePath, &items));
    QCOMPARE(items.size(), std::size_t(0));

    canvas.undo();
    QVERIFY(canvas.saveFukeFile(filePath));
    QVERIFY(CanvasFile::load(filePath, &items));
    QCOMPARE(items.size(), std::size_t(1));
}

void CanvasWidgetTest::copyAndPasteSelectionAssignsFreshIds()
{
    CanvasWidget canvas;
    canvas.resize(400, 300);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    QTest::mousePress(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));
    QTest::mouseMove(&canvas, QPoint(120, 50));
    QTest::mouseRelease(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(120, 50));
    canvas.setMode(Mode::Select);
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(80, 50));
    canvas.setFocus();
    QTest::keyClick(&canvas, Qt::Key_C, Qt::ControlModifier);
    QTest::keyClick(&canvas, Qt::Key_V, Qt::ControlModifier);

    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString filePath = temporaryDirectory.filePath("board.fuke");
    QVERIFY(canvas.saveFukeFile(filePath));

    CanvasItemList items;
    QVERIFY(CanvasFile::load(filePath, &items));
    QCOMPARE(items.size(), std::size_t(2));
    QCOMPARE(items[0]->id(), CanvasItemId(1));
    QCOMPARE(items[1]->id(), CanvasItemId(2));
    QCOMPARE(items[1]->boundingRect().topLeft() - items[0]->boundingRect().topLeft(), QPointF(30, 30));

    QTest::keyClick(&canvas, Qt::Key_V, Qt::ControlModifier);
    QVERIFY(canvas.saveFukeFile(filePath));
    QVERIFY(CanvasFile::load(filePath, &items));
    QCOMPARE(items.size(), std::size_t(3));
    QCOMPARE(items[2]->id(), CanvasItemId(3));
    QCOMPARE(items[2]->boundingRect().topLeft() - items[0]->boundingRect().topLeft(), QPointF(60, 60));

    QTest::keyClick(&canvas, Qt::Key_Delete);
    QVERIFY(canvas.saveFukeFile(filePath));
    QVERIFY(CanvasFile::load(filePath, &items));
    QCOMPARE(items.size(), std::size_t(2));

    canvas.undo();
    QVERIFY(canvas.saveFukeFile(filePath));
    QVERIFY(CanvasFile::load(filePath, &items));
    QCOMPARE(items.size(), std::size_t(3));
}

void CanvasWidgetTest::copyAndPasteMultipleSelectedItems()
{
    CanvasWidget canvas;
    canvas.resize(400, 300);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    QTest::mousePress(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));
    QTest::mouseMove(&canvas, QPoint(120, 50));
    QTest::mouseRelease(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(120, 50));
    QTest::mousePress(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(50, 100));
    QTest::mouseMove(&canvas, QPoint(120, 100));
    QTest::mouseRelease(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(120, 100));

    canvas.setMode(Mode::Select);
    QTest::mousePress(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(30, 30));
    QTest::mouseMove(&canvas, QPoint(140, 120));
    QTest::mouseRelease(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(140, 120));
    canvas.setFocus();
    QTest::keyClick(&canvas, Qt::Key_C, Qt::ControlModifier);
    QTest::keyClick(&canvas, Qt::Key_V, Qt::ControlModifier);

    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString filePath = temporaryDirectory.filePath("board.fuke");
    QVERIFY(canvas.saveFukeFile(filePath));

    CanvasItemList items;
    QVERIFY(CanvasFile::load(filePath, &items));
    QCOMPARE(items.size(), std::size_t(4));

    std::set<CanvasItemId> ids;
    for (const auto &item : items) {
        ids.insert(item->id());
    }
    QCOMPARE(ids.size(), std::size_t(4));
}

void CanvasWidgetTest::copyAndPasteImageItemPreservesPixels()
{
    CanvasWidget canvas;
    canvas.resize(400, 300);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    QImage image(4, 4, QImage::Format_ARGB32);
    image.fill(Qt::red);
    QApplication::clipboard()->setImage(image);
    QCoreApplication::processEvents();
    canvas.setFocus();
    QTest::keyClick(&canvas, Qt::Key_V, Qt::ControlModifier);

    canvas.setMode(Mode::Select);
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(200, 150));
    canvas.setFocus();
    QTest::keyClick(&canvas, Qt::Key_C, Qt::ControlModifier);
    QTest::keyClick(&canvas, Qt::Key_V, Qt::ControlModifier);

    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString filePath = temporaryDirectory.filePath("board.fuke");
    QVERIFY(canvas.saveFukeFile(filePath));

    CanvasItemList items;
    QVERIFY(CanvasFile::load(filePath, &items));
    QCOMPARE(items.size(), std::size_t(2));

    for (const auto &item : items) {
        auto *imageItem = dynamic_cast<ImageItem*>(item.get());
        QVERIFY(imageItem);
        QCOMPARE(imageItem->image().pixelColor(0, 0), QColor(Qt::red));
    }
}

void CanvasWidgetTest::externalClipboardImageInvalidatesSelectionCopy()
{
    CanvasWidget canvas;
    canvas.resize(400, 300);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    QTest::mousePress(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));
    QTest::mouseMove(&canvas, QPoint(120, 50));
    QTest::mouseRelease(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(120, 50));
    canvas.setMode(Mode::Select);
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(80, 50));
    QVERIFY(canvas.copySelection());

    QImage image(4, 4, QImage::Format_ARGB32);
    image.fill(Qt::red);
    QApplication::clipboard()->setImage(image);
    QCoreApplication::processEvents();

    canvas.setFocus();
    QTest::keyClick(&canvas, Qt::Key_V, Qt::ControlModifier);

    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString filePath = temporaryDirectory.filePath("board.fuke");
    QVERIFY(canvas.saveFukeFile(filePath));

    CanvasItemList items;
    QVERIFY(CanvasFile::load(filePath, &items));
    QCOMPARE(items.size(), std::size_t(2));
    QVERIFY(dynamic_cast<ImageItem*>(items[1].get()));
}

QTEST_MAIN(CanvasWidgetTest)
#include "tst_canvaswidget.moc"
