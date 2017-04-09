QT += core network concurrent
QT -= gui

CONFIG += c++11

TEMPLATE = app

include( $$PWD/../../../library/JQLibrary/JQLibrary.pri )

SOURCES += \
    $$PWD/cpp/main.cpp

RESOURCES += \
    $$PWD/key/key.qrc
