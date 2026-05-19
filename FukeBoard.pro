TEMPLATE = app
TARGET = FukeBoard

QT = core gui widgets

CONFIG += c++20

SOURCES += \
    canvasitem.cpp \
    canvaswidget.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    appconstants.h \
    canvasitem.h \
    canvaswidget.h \
    mainwindow.h

DISTFILES += \
    thingslefttomake.md
