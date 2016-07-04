QT += core network
QT -= gui

CONFIG += c++11
CONFIG += c++14

TARGET = ClientTest

TEMPLATE = app

include($$PWD/../SharedLibrary/JQLibrary/JQLibrary.pri)

SOURCES += main.cpp
