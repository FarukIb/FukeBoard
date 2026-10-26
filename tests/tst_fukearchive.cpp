#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

#include "fukearchive.h"

class FukeArchiveTest : public QObject
{
    Q_OBJECT

private slots:
    void writesAndReadsEntries();
    void rejectsInvalidArchive();
};

void FukeArchiveTest::writesAndReadsEntries()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString filePath = temporaryDirectory.filePath("board.fuke");
    const QMap<QString, QByteArray> expected{
        {"document.json", R"({"version":1})"},
        {"assets/image-1.png", "image data"},
    };

    QVERIFY(FukeArchive::write(filePath, expected));

    QMap<QString, QByteArray> actual;
    QVERIFY(FukeArchive::read(filePath, &actual));
    QVERIFY(actual == expected);
}

void FukeArchiveTest::rejectsInvalidArchive()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString filePath = temporaryDirectory.filePath("invalid.fuke");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.write("not a Fuke archive") > 0);

    QMap<QString, QByteArray> entries;
    QVERIFY(!FukeArchive::read(filePath, &entries));
}

QTEST_MAIN(FukeArchiveTest)
#include "tst_fukearchive.moc"
