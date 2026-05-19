TEMPLATE = app
TARGET = FukeBoard

QT = core gui widgets

CONFIG += c++20

SOURCES += \
    canvasitem.cpp \
    canvaswidget.cpp \
    main.cpp \
    mainwindow.cpp \
    selectioncontroller.cpp

HEADERS += \
    appconstants.h \
    canvasitem.h \
    canvaswidget.h \
    hitbox.h \
    mainwindow.h \
    selectioncontroller.h

DISTFILES += \
    thingslefttomake.md
