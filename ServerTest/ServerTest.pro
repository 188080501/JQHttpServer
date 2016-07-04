QT += core network concurrent
QT -= gui

CONFIG += c++11
CONFIG += c++14

TARGET = ServerTest

TEMPLATE = app

include($$PWD/../SharedLibrary/JQLibrary/JQLibrary.pri)

SOURCES += main.cpp
