#include "fukearchive.h"

#include <QFile>
#include <QIODevice>
#include <QList>
#include <QtEndian>

#include <limits>

namespace {
constexpr quint32 LocalFileHeaderSignature = 0x04034b50;
constexpr quint32 CentralDirectoryHeaderSignature = 0x02014b50;
constexpr quint32 EndOfCentralDirectorySignature = 0x06054b50;
constexpr quint16 StoreMethod = 0;

struct CentralDirectoryEntry {
    QString name;
    QByteArray nameBytes;
    quint32 crc = 0;
    quint32 size = 0;
    quint32 localHeaderOffset = 0;
};

void appendUInt16(QByteArray &bytes, quint16 value)
{
    char buffer[2];
    qToLittleEndian(value, buffer);
    bytes.append(buffer, 2);
}

void appendUInt32(QByteArray &bytes, quint32 value)
{
    char buffer[4];
    qToLittleEndian(value, buffer);
    bytes.append(buffer, 4);
}

quint16 readUInt16(const QByteArray &bytes, qsizetype offset)
{
    return qFromLittleEndian<quint16>(bytes.constData() + offset);
}

quint32 readUInt32(const QByteArray &bytes, qsizetype offset)
{
    return qFromLittleEndian<quint32>(bytes.constData() + offset);
}

quint32 crc32(const QByteArray &bytes)
{
    quint32 crc = 0xffffffffU;

    for (unsigned char byte : bytes) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            const quint32 mask = -(crc & 1U);
            crc = (crc >> 1) ^ (0xedb88320U & mask);
        }
    }

    return ~crc;
}

bool appendStoredEntry(QByteArray &archive, CentralDirectoryEntry &entry, const QByteArray &content)
{
    entry.nameBytes = entry.name.toUtf8();
    if (entry.nameBytes.size() > std::numeric_limits<quint16>::max() ||
        content.size() > std::numeric_limits<quint32>::max()) {
        return false;
    }

    entry.crc = crc32(content);
    entry.size = static_cast<quint32>(content.size());
    entry.localHeaderOffset = static_cast<quint32>(archive.size());

    appendUInt32(archive, LocalFileHeaderSignature);
    appendUInt16(archive, 20);
    appendUInt16(archive, 0);
    appendUInt16(archive, StoreMethod);
    appendUInt16(archive, 0);
    appendUInt16(archive, 0);
    appendUInt32(archive, entry.crc);
    appendUInt32(archive, entry.size);
    appendUInt32(archive, entry.size);
    appendUInt16(archive, static_cast<quint16>(entry.nameBytes.size()));
    appendUInt16(archive, 0);
    archive.append(entry.nameBytes);
    archive.append(content);

    return true;
}

void appendCentralDirectory(QByteArray &archive, const QList<CentralDirectoryEntry> &entries, quint32 startOffset)
{
    for (const CentralDirectoryEntry &entry : entries) {
        appendUInt32(archive, CentralDirectoryHeaderSignature);
        appendUInt16(archive, 20);
        appendUInt16(archive, 20);
        appendUInt16(archive, 0);
        appendUInt16(archive, StoreMethod);
        appendUInt16(archive, 0);
        appendUInt16(archive, 0);
        appendUInt32(archive, entry.crc);
        appendUInt32(archive, entry.size);
        appendUInt32(archive, entry.size);
        appendUInt16(archive, static_cast<quint16>(entry.nameBytes.size()));
        appendUInt16(archive, 0);
        appendUInt16(archive, 0);
        appendUInt16(archive, 0);
        appendUInt16(archive, 0);
        appendUInt32(archive, 0);
        appendUInt32(archive, entry.localHeaderOffset);
        archive.append(entry.nameBytes);
    }

    const quint32 centralDirectorySize = static_cast<quint32>(archive.size()) - startOffset;

    appendUInt32(archive, EndOfCentralDirectorySignature);
    appendUInt16(archive, 0);
    appendUInt16(archive, 0);
    appendUInt16(archive, static_cast<quint16>(entries.size()));
    appendUInt16(archive, static_cast<quint16>(entries.size()));
    appendUInt32(archive, centralDirectorySize);
    appendUInt32(archive, startOffset);
    appendUInt16(archive, 0);
}
}

namespace FukeArchive {

bool write(const QString &filePath, const QMap<QString, QByteArray> &entries)
{
    QByteArray archive;
    QList<CentralDirectoryEntry> centralDirectory;
    centralDirectory.reserve(entries.size());

    for (auto it = entries.constBegin(); it != entries.constEnd(); ++it) {
        CentralDirectoryEntry entry;
        entry.name = it.key();
        if (!appendStoredEntry(archive, entry, it.value())) {
            return false;
        }
        centralDirectory.append(entry);
    }

    const quint32 centralDirectoryOffset = static_cast<quint32>(archive.size());
    appendCentralDirectory(archive, centralDirectory, centralDirectoryOffset);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    return file.write(archive) == archive.size();
}

bool read(const QString &filePath, QMap<QString, QByteArray> *entries)
{
    if (!entries) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QByteArray archive = file.readAll();
    entries->clear();

    qsizetype offset = 0;
    while (offset + 30 <= archive.size()) {
        const quint32 signature = readUInt32(archive, offset);
        if (signature == CentralDirectoryHeaderSignature || signature == EndOfCentralDirectorySignature) {
            return true;
        }

        if (signature != LocalFileHeaderSignature) {
            return false;
        }

        const quint16 method = readUInt16(archive, offset + 8);
        const quint32 crc = readUInt32(archive, offset + 14);
        const quint32 compressedSize = readUInt32(archive, offset + 18);
        const quint32 uncompressedSize = readUInt32(archive, offset + 22);
        const quint16 nameLength = readUInt16(archive, offset + 26);
        const quint16 extraLength = readUInt16(archive, offset + 28);

        if (method != StoreMethod || compressedSize != uncompressedSize) {
            return false;
        }

        const qsizetype nameOffset = offset + 30;
        const qsizetype dataOffset = nameOffset + nameLength + extraLength;
        const qsizetype nextOffset = dataOffset + compressedSize;
        if (dataOffset > archive.size() || nextOffset > archive.size()) {
            return false;
        }

        const QString name = QString::fromUtf8(archive.mid(nameOffset, nameLength));
        const QByteArray content = archive.mid(dataOffset, compressedSize);
        if (crc32(content) != crc) {
            return false;
        }

        entries->insert(name, content);
        offset = nextOffset;
    }

    return false;
}

}
