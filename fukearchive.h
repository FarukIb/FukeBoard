#ifndef FUKEARCHIVE_H
#define FUKEARCHIVE_H

#include <QByteArray>
#include <QMap>
#include <QString>

namespace FukeArchive {

bool write(const QString &filePath, const QMap<QString, QByteArray> &entries);
bool read(const QString &filePath, QMap<QString, QByteArray> *entries);

}

#endif // FUKEARCHIVE_H
