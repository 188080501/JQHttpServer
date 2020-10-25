QT += core network concurrent testlib
QT -= gui

CONFIG += c++11

TEMPLATE = app

include($$PWD/../../library/JQLibrary/JQLibrary.pri)

HEADERS += \
    $$PWD/cpp/*.h

SOURCES += \
    $$PWD/cpp/*.cpp
