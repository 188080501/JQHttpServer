﻿#include "benchmark.h"

// Qt lib import
#include <QtTest>
#include <QSemaphore>
#include <QtConcurrent>

// JQLibrary import
#include <JQHttpServer>
#include <JQNet>

void BenchMark::initTestCase()
{
    tcpServerManage_.reset( new JQHttpServer::TcpServerManage );

    tcpServerManage_->setHttpAcceptedCallback( [ ]( const QPointer< JQHttpServer::Session > &session )
    {
        session->replyText( QString( "->%1<-->%2<-" ).arg( session->requestUrl(), QString( session->requestBody() ) ) );
    } );

    QCOMPARE( tcpServerManage_->listen( QHostAddress::Any, 23413 ), true );
}

void BenchMark::cleanupTestCase()
{
    tcpServerManage_.clear();
}

void BenchMark::benchMarkForOnce()
{
    QBENCHMARK
    {
        const auto &&reply = JQNet::HTTP::post( "http://127.0.0.1:23413/test/", "append data" );
        QCOMPARE( reply.first, true );
        QCOMPARE( reply.second, QByteArray( "->/test/<-->append data<-" ) );
    }
}

void BenchMark::benchMarkFor5000()
{
    QBENCHMARK_ONCE
    {
        for ( auto testCount = 0; testCount < 5000; ++testCount )
        {
            const auto &&reply = JQNet::HTTP::post( "http://127.0.0.1:23413/test/", "append data" );
            QCOMPARE( reply.first, true );
            QCOMPARE( reply.second, QByteArray( "->/test/<-->append data<-" ) );
        }
    }
}
