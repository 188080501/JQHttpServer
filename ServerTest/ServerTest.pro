QT += core network concurrent
QT -= gui

CONFIG += c++11

TARGET = ServerTest

TEMPLATE = app

include($$PWD/../SharedLibrary/JQLibrary/JQLibrary.pri)

SOURCES += main.cpp
