// Qt lib import
#include <QtCore>

// JQLibrary import
#include "JQHttpServer.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    JQHttpServer::Manage httpServerManage;

    httpServerManage.setHttpAcceptedCallback( []( const auto &session )
    {
        session->replyText( QString( "->%1<-->%2<-" ).arg( session->requestUrl(), QString( session->requestRawData() ) ) );
    } );

    qDebug() << "listen:" << httpServerManage.listen( QHostAddress::Any, 23412 );

    return a.exec();
}
