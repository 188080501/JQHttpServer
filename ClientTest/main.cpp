// Qt lib import
#include <QtCore>

// JQLibrary import
#include "JQNet.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    const auto &&reply = JQNet::HTTP::post( "http://127.0.0.1:23412/TestUrl", "AppendData" );
//    const auto &&reply = JQNet::HTTP::get( "http://127.0.0.1:23412/TestUrl" );

    qDebug() << reply.first << reply.second;

    return 0;
}
