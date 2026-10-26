#include <QtTest>

#include <QImage>
#include <QTemporaryDir>

#include "canvasdocument.h"
#include "canvasfile.h"
#include "imageitem.h"
#include "textboxitem.h"

class CanvasComponentsTest : public QObject
{
    Q_OBJECT

private slots:
    void documentAssignsIdsAndMaintainsZOrder();
    void documentRestoresSnapshots();
    void fileRoundTripPreservesItemsAndAssets();
    void fileRejectsMissingOrInvalidDocuments();
};

void CanvasComponentsTest::documentAssignsIdsAndMaintainsZOrder()
{
    CanvasDocument document;

    auto bottom = std::make_unique<TextBoxItem>(QRectF(0, 0, 100, 40), "bottom");
    auto top = std::make_unique<TextBoxItem>(QRectF(0, 0, 100, 40), "top");
    CanvasItem &bottomItem = document.addItem(std::move(bottom));
    CanvasItem &topItem = document.addItem(std::move(top));

    QCOMPARE(bottomItem.id(), CanvasItemId(1));
    QCOMPARE(topItem.id(), CanvasItemId(2));
    QCOMPARE(document.itemAt(QPointF(20, 20)), &topItem);

    document.bringToFront(&bottomItem);
    QCOMPARE(document.itemAt(QPointF(20, 20)), &bottomItem);
    QVERIFY(document.boundingRect().contains(QPointF(20, 20)));
}

void CanvasComponentsTest::documentRestoresSnapshots()
{
    CanvasDocument document;
    document.addItem(std::make_unique<TextBoxItem>(QRectF(10, 20, 100, 40), "text"));

    CanvasItemList snapshot = document.cloneItems();
    QVERIFY(document.snapshotsEquivalent(snapshot, document.cloneItems()));

    document.items().front()->moveBy(QPointF(30, 0));
    QVERIFY(!document.snapshotsEquivalent(snapshot, document.cloneItems()));

    document.restoreSnapshot(snapshot);
    QCOMPARE(document.items().size(), std::size_t(1));
    QCOMPARE(document.items().front()->id(), CanvasItemId(1));
    QCOMPARE(document.items().front()->boundingRect().left(), snapshot.front()->boundingRect().left());
}

void CanvasComponentsTest::fileRoundTripPreservesItemsAndAssets()
{
    CanvasDocument document;
    document.addItem(std::make_unique<TextBoxItem>(QRectF(10, 20, 100, 40), "<b>Hello</b>"));

    QImage image(2, 2, QImage::Format_ARGB32);
    image.fill(Qt::red);
    document.addItem(std::make_unique<ImageItem>(QRectF(30, 40, 80, 60), image));

    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString filePath = temporaryDirectory.filePath("board.fuke");
    QVERIFY(CanvasFile::save(filePath, document.items()));

    CanvasItemList loadedItems;
    QVERIFY(CanvasFile::load(filePath, &loadedItems));
    QCOMPARE(loadedItems.size(), std::size_t(2));
    QCOMPARE(loadedItems[0]->id(), CanvasItemId(1));
    QCOMPARE(loadedItems[1]->id(), CanvasItemId(2));

    auto *loadedText = dynamic_cast<TextBoxItem *>(loadedItems[0].get());
    auto *loadedImage = dynamic_cast<ImageItem *>(loadedItems[1].get());
    QVERIFY(loadedText);
    QVERIFY(loadedImage);
    QCOMPARE(loadedText->plainText(), QString("Hello"));
    QCOMPARE(loadedImage->image().pixelColor(0, 0), QColor(Qt::red));
}

void CanvasComponentsTest::fileRejectsMissingOrInvalidDocuments()
{
    CanvasItemList loadedItems;
    QVERIFY(!CanvasFile::load("/path/that/does/not/exist.fuke", &loadedItems));
    QVERIFY(!CanvasFile::load("/path/that/does/not/exist.fuke", nullptr));
}

QTEST_MAIN(CanvasComponentsTest)
#include "tst_canvas_components.moc"
