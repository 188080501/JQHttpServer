QT += core network concurrent testlib
QT -= gui

CONFIG += c++11

TEMPLATE = app

include($$PWD/../../library/JQLibrary/JQLibrary.pri)

HEADERS += \
    $$PWD/cpp/OverallTest.h

SOURCES += \
    $$PWD/cpp/main.cpp \
    $$PWD/cpp/OverallTest.cpp

RESOURCES += \
    $$PWD/key/key.qrc
