// Qt lib import
#include <QCoreApplication>
#include <QtTest>

// Project lib import
#include "overalltest.h"

int main(int argc, char *argv[])
{
    qputenv( "QT_SSL_USE_TEMPORARY_KEYCHAIN", "1" );
    qSetMessagePattern( "%{time hh:mm:ss.zzz}: %{message}" );

    QCoreApplication app( argc, argv );

    OverallTest benchMark;

    return QTest::qExec( &benchMark, argc, argv );
}
