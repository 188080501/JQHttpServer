QT += core network concurrent testlib
QT -= gui

CONFIG += c++11
CONFIG += c++14

TARGET = BenchMark

TEMPLATE = app

include($$PWD/../SharedLibrary/JQLibrary/JQLibrary.pri)

SOURCES += $$PWD/cpp/main.cpp \
    $$PWD/cpp/BenchMark.cpp

HEADERS += $$PWD/cpp/BenchMark.h
