#include "canvasfile.h"

#include "fukearchive.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace CanvasFile {

bool save(const QString &filePath, const CanvasItemList &items)
{
    CanvasSerializationContext context;
    QJsonArray itemsJson;

    for (const auto &item : items) {
        if (item) {
            itemsJson.append(item->serialize(context));
        }
    }

    const QJsonObject root {
        {"format", "FukeBoard"},
        {"version", 1},
        {"items", itemsJson}
    };

    QMap<QString, QByteArray> entries = context.assets;
    entries.insert("canvas.json", QJsonDocument(root).toJson(QJsonDocument::Indented));
    return FukeArchive::write(filePath, entries);
}

bool load(const QString &filePath, CanvasItemList *items)
{
    if (!items) {
        return false;
    }

    QMap<QString, QByteArray> entries;
    if (!FukeArchive::read(filePath, &entries) || !entries.contains("canvas.json")) {
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(entries.value("canvas.json"), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return false;
    }

    const QJsonObject root = document.object();
    if (root.value("format").toString() != "FukeBoard") {
        return false;
    }

    CanvasDeserializationContext context;
    context.assets = entries;

    CanvasItemList loadedItems;
    const QJsonArray itemsJson = root.value("items").toArray();
    loadedItems.reserve(static_cast<std::size_t>(itemsJson.size()));

    for (const QJsonValue &itemValue : itemsJson) {
        std::unique_ptr<CanvasItem> item = CanvasItem::deserializeItem(itemValue.toObject(), context);
        if (!item) {
            return false;
        }
        loadedItems.push_back(std::move(item));
    }

    *items = std::move(loadedItems);
    return true;
}

}
