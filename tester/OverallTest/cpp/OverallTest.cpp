#include "OverallTest.h"

// Qt lib import
#include <QtTest>
#include <QSemaphore>
#include <QtConcurrent>

// JQLibrary import
#include "JQHttpServer.h"
#include "JQNet.h"

void OverallTest::initTestCase()
{
    tcpServerManage_.reset( new JQHttpServer::TcpServerManage );

    tcpServerManage_->setHttpAcceptedCallback( [ ]( const QPointer< JQHttpServer::Session > &session )
    {
        session->replyText( QString( "->%1<-->%2<-" ).arg( session->requestUrl(), QString( session->requestRawData() ) ) );
    } );

    QCOMPARE( (bool)tcpServerManage_->listen( QHostAddress::Any, 24680 ), true );
}

void OverallTest::cleanupTestCase()
{
    tcpServerManage_->close();
}
