﻿// Qt lib import
#include <QCoreApplication>
#include <QtTest>

// Project lib import
#include "OverallTest.h"

int main(int argc, char *argv[])
{
    QCoreApplication app( argc, argv );

    OverallTest benchMark;

    return QTest::qExec( &benchMark, argc, argv );
}
