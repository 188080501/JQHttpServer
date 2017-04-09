// Qt lib import
#include <QtCore>

// JQLibrary import
#include "JQNet.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    const auto &&reply = JQNet::HTTP::post( "https://127.0.0.1:24684/TestUrl", "AppendData" );
//    const auto &&reply = JQNet::HTTP::get( "https://127.0.0.1:24684/TestUrl" );

    qDebug() << reply.first << reply.second;

    return 0;
}
