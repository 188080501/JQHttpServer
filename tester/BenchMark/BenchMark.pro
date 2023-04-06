QT += core network concurrent testlib
QT -= gui

CONFIG += c++11

TEMPLATE = app

include($$PWD/../../library/JQLibrary/JQLibrary.pri)

HEADERS += \
    $$PWD/cpp/benchmark.h

SOURCES += \
    $$PWD/cpp/benchmark.cpp \
    $$PWD/cpp/main.cpp
