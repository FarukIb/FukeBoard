#ifndef CANVASCOMMAND_H
#define CANVASCOMMAND_H

#include "canvasitem.h"

#include <memory>
#include <vector>

class CanvasWidget;

using CanvasItemList = std::vector<std::unique_ptr<CanvasItem>>;

class CanvasCommand
{
public:
    virtual ~CanvasCommand() = default;

    virtual void undo(CanvasWidget &canvas) = 0;
    virtual void redo(CanvasWidget &canvas) = 0;
};

class CanvasSnapshotCommand : public CanvasCommand
{
public:
    CanvasSnapshotCommand(CanvasItemList before, CanvasItemList after);

    void undo(CanvasWidget &canvas) override;
    void redo(CanvasWidget &canvas) override;

private:
    CanvasItemList m_before;
    CanvasItemList m_after;
};

#endif // CANVASCOMMAND_H
