#include "canvascommand.h"

#include "canvaswidget.h"

CanvasSnapshotCommand::CanvasSnapshotCommand(CanvasItemList before, CanvasItemList after)
    : m_before(std::move(before)), m_after(std::move(after))
{
}

void CanvasSnapshotCommand::undo(CanvasWidget &canvas)
{
    canvas.restoreItemsFromSnapshot(m_before);
}

void CanvasSnapshotCommand::redo(CanvasWidget &canvas)
{
    canvas.restoreItemsFromSnapshot(m_after);
}
