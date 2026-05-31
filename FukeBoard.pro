TEMPLATE = app
TARGET = FukeBoard

QT = core gui widgets

CONFIG += c++20

SOURCES += \
    canvascommand.cpp \
    canvasitem.cpp \
    canvaswidget.cpp \
    imageitem.cpp \
    main.cpp \
    mainwindow.cpp \
    milegriditem.cpp \
    selectioncontroller.cpp \
    strokeitem.cpp \
    textboxitem.cpp

HEADERS += \
    appconstants.h \
    canvascommand.h \
    canvasitem.h \
    canvaswidget.h \
    hitbox.h \
    imageitem.h \
    mainwindow.h \
    milegriditem.h \
    selectioncontroller.h \
    strokeitem.h \
    textboxitem.h

DISTFILES += \
    thingslefttomake.md
