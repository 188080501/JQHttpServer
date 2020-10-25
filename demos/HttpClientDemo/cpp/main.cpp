// Qt lib import
#include <QtCore>

// JQLibrary import
#include <JQNet>

int main(int argc, char *argv[])
{
    qputenv( "QT_SSL_USE_TEMPORARY_KEYCHAIN", "1" );
    qSetMessagePattern( "%{time hh:mm:ss.zzz}: %{message}" );

    QCoreApplication app( argc, argv );

    const auto &&reply = JQNet::HTTP::post( "http://127.0.0.1:23412/TestUrl", "BodyData" );
    qDebug() << "HTTP post reply:" << reply.first << reply.second;

    const auto &&reply2 = JQNet::HTTP::post( "https://127.0.0.1:23413/TestUrl", "BodyData" );
    qDebug() << "HTTPS post reply:" << reply2.first << reply2.second;

    return 0;
}
