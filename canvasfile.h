#ifndef CANVASFILE_H
#define CANVASFILE_H

#include "canvasitem.h"

namespace CanvasFile {

// Converts a document to and from the on-disk .fuke format.
bool save(const QString &filePath, const CanvasItemList &items);
bool load(const QString &filePath, CanvasItemList *items);

}

#endif // CANVASFILE_H
