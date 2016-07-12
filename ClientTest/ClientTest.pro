QT += core network
QT -= gui

CONFIG += c++11

TARGET = ClientTest

TEMPLATE = app

include($$PWD/../SharedLibrary/JQLibrary/JQLibrary.pri)

SOURCES += main.cpp
