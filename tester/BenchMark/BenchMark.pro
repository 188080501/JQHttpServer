QT += core network concurrent testlib
QT -= gui

CONFIG += c++11

TEMPLATE = app

include($$PWD/../../library/JQLibrary/JQLibrary.pri)

HEADERS += \
    $$PWD/cpp/BenchMark.h

SOURCES += \
    $$PWD/cpp/main.cpp \
    $$PWD/cpp/BenchMark.cpp
