#include "OverallTest.h"

// Qt lib import
#include <QtTest>
#include <QSemaphore>
#include <QTcpSocket>
#include <QtConcurrent>

// JQLibrary import
#include "JQHttpServer.h"
#include "JQNet.h"

void OverallTest::initTestCase()
{
    httpServerManage_.reset( new JQHttpServer::TcpServerManage );

    httpServerManage_->setHttpAcceptedCallback( [ ]( const QPointer< JQHttpServer::Session > &session )
    {
        session->replyText( QString( "->%1<-->%2<-" ).arg( session->requestUrl(), QString( session->requestBody() ) ) );
    } );

    QCOMPARE( httpServerManage_->listen( QHostAddress::Any, 24680 ), true );

#ifndef QT_NO_SSL
    httpsServerManage_.reset( new JQHttpServer::SslServerManage );

    httpsServerManage_->setHttpAcceptedCallback( [ ]( const QPointer< JQHttpServer::Session > &session )
    {
        session->replyText( QString( "->%1<-->%2<-" ).arg( session->requestUrl(), QString( session->requestBody() ) ) );
    } );

    QCOMPARE( httpsServerManage_->listen( QHostAddress::Any, 24681, ":/server.crt", ":/server.key" ), true );
#endif
}

void OverallTest::cleanupTestCase()
{
    httpServerManage_.clear();
#ifndef QT_NO_SSL
    httpsServerManage_.clear();
#endif
}

void OverallTest::connectTest()
{
    for ( auto index = 0; index < 200; ++index )
    {
        QTcpSocket socket;

        socket.connectToHost( "127.0.0.1", 24680 );
        QCOMPARE( socket.waitForConnected( 1000 ), true );

        socket.write( "HTTP" );
        QCOMPARE( socket.waitForBytesWritten( 1000 ), true );
    }
}

void OverallTest::httpGetTest()
{
    const auto &&reply = JQNet::HTTP::get( "http://127.0.0.1:24680/httpGetTest/" );
    QCOMPARE( reply.first, true );
    QCOMPARE( reply.second, QByteArray( "->/httpGetTest/<--><-" ) );
}

void OverallTest::httpPostTest()
{
    const auto &&reply = JQNet::HTTP::post( "http://127.0.0.1:24680/httpPostTest/", "append data" );
    QCOMPARE( reply.first, true );
    QCOMPARE( reply.second, QByteArray( "->/httpPostTest/<-->append data<-" ) );
}

#ifndef QT_NO_SSL
void OverallTest::httpsGetTest()
{
    const auto &&reply = JQNet::HTTP::get( "https://127.0.0.1:24681/httpGetTest/" );
    QCOMPARE( reply.first, true );
    QCOMPARE( reply.second, QByteArray( "->/httpGetTest/<--><-" ) );
}

void OverallTest::httpsPostTest()
{
    const auto &&reply = JQNet::HTTP::post( "https://127.0.0.1:24681/httpPostTest/", "append data" );
    QCOMPARE( reply.first, true );
    QCOMPARE( reply.second, QByteArray( "->/httpPostTest/<-->append data<-" ) );
}
#endif
