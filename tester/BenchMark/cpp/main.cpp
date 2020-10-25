// Qt lib import
#include <QCoreApplication>
#include <QtTest>

// Project lib import
#include "benchmark.h"

int main(int argc, char *argv[])
{
    QCoreApplication app( argc, argv );

    BenchMark benchMark;

    return QTest::qExec( &benchMark, argc, argv );
}
