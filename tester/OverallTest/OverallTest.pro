QT += core network concurrent testlib
QT -= gui

CONFIG += c++11

TEMPLATE = app

include($$PWD/../../library/JQLibrary/JQLibrary.pri)

HEADERS += \
    $$PWD/cpp/overalltest.h

SOURCES += \
    $$PWD/cpp/overalltest.cpp \
    $$PWD/cpp/main.cpp

RESOURCES += \
    $$PWD/key/key.qrc
