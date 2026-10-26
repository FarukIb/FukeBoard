#include "canvasdocument.h"

#include <algorithm>
#include <cmath>

namespace {
bool fuzzyRectEquals(const QRectF &left, const QRectF &right)
{
    constexpr qreal Epsilon = 0.001;

    return std::abs(left.left() - right.left()) < Epsilon &&
           std::abs(left.top() - right.top()) < Epsilon &&
           std::abs(left.width() - right.width()) < Epsilon &&
           std::abs(left.height() - right.height()) < Epsilon;
}
}

const CanvasItemList &CanvasDocument::items() const
{
    return m_items;
}

CanvasItemList &CanvasDocument::items()
{
    return m_items;
}

CanvasItem &CanvasDocument::addItem(std::unique_ptr<CanvasItem> item)
{
    assignFreshId(*item);
    m_items.push_back(std::move(item));
    return *m_items.back();
}

void CanvasDocument::assignFreshId(CanvasItem &item)
{
    item.setId(m_nextItemId++);
}

void CanvasDocument::replaceItems(CanvasItemList items)
{
    m_items = std::move(items);
    updateNextItemId();
}

void CanvasDocument::restoreSnapshot(const CanvasItemList &snapshot)
{
    m_items.clear();
    m_items.reserve(snapshot.size());

    for (const auto &item : snapshot) {
        if (item) {
            m_items.push_back(item->clone());
        }
    }

    updateNextItemId();
}

CanvasItem *CanvasDocument::itemAt(const QPointF &scenePos) const
{
    for (auto itemIt = m_items.rbegin(); itemIt != m_items.rend(); ++itemIt) {
        if (*itemIt && (*itemIt)->contains(scenePos)) {
            return itemIt->get();
        }
    }

    return nullptr;
}

CanvasItem *CanvasDocument::itemById(CanvasItemId id) const
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

void CanvasDocument::bringToFront(CanvasItem *item)
{
    const auto itemIt = std::find_if(
        m_items.begin(), m_items.end(),
        [item](const std::unique_ptr<CanvasItem> &candidate) {
            return candidate.get() == item;
        });

    if (itemIt == m_items.end() || std::next(itemIt) == m_items.end()) {
        return;
    }

    std::unique_ptr<CanvasItem> movedItem = std::move(*itemIt);
    m_items.erase(itemIt);
    m_items.push_back(std::move(movedItem));
}

QRectF CanvasDocument::boundingRect() const
{
    QRectF bounds;

    for (const auto &item : m_items) {
        if (!item || item->boundingRect().isEmpty()) {
            continue;
        }

        bounds = bounds.isNull() ? item->boundingRect() : bounds.united(item->boundingRect());
    }

    return bounds;
}

CanvasItemList CanvasDocument::cloneItems() const
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

bool CanvasDocument::snapshotsEquivalent(
    const CanvasItemList &left,
    const CanvasItemList &right
    ) const
{
    if (left.size() != right.size()) {
        return false;
    }

    for (std::size_t i = 0; i < left.size(); ++i) {
        const CanvasItem *leftItem = left[i].get();
        const CanvasItem *rightItem = right[i].get();

        if (!leftItem || !rightItem) {
            if (leftItem != rightItem) {
                return false;
            }
            continue;
        }

        if (leftItem->id() != rightItem->id() ||
            !fuzzyRectEquals(leftItem->boundingRect(), rightItem->boundingRect())) {
            return false;
        }
    }

    return true;
}

void CanvasDocument::updateNextItemId()
{
    CanvasItemId maxId = InvalidCanvasItemId;
    for (const auto &item : m_items) {
        if (item) {
            maxId = std::max(maxId, item->id());
        }
    }
    m_nextItemId = maxId + 1;
}
