#ifndef APPCONSTANTS_H
#define APPCONSTANTS_H

#include <QColor>

namespace AppConstants {
constexpr int DefaultPenWidth = 3;
const QColor DefaultPenColor = Qt::black;
}

enum class Mode {
    Pen,
    Erase,
    Select
};

#endif // APPCONSTANTS_H