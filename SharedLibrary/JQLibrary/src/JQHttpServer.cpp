/*
    This file is part of JQLibrary

    Copyright: Jason

    Contact email: 188080501@qq.com

    GNU Lesser General Public License Usage
    Alternatively, this file may be used under the terms of the GNU Lesser
    General Public License version 2.1 or version 3 as published by the Free
    Software Foundation and appearing in the file LICENSE.LGPLv21 and
    LICENSE.LGPLv3 included in the packaging of this file. Please review the
    following information to ensure the GNU Lesser General Public License
    requirements will be met: https://www.gnu.org/licenses/lgpl.html and
    http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
*/

#include "JQHttpServer.h"

// Qt lib import
#include <QEventLoop>
#include <QTimer>
#include <QSemaphore>
#include <QMetaObject>
#include <QThread>
#include <QThreadPool>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtConcurrent>

using namespace JQHttpServer;

static QString replyTextFormat(
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html;charset=UTF-8\r\n"
        "Content-Length: %1\r\n"
        "\r\n"
        "%2"
    );

// Session
Session::Session(const QPointer<QTcpSocket> &tcpSocket):
    tcpSocket_( tcpSocket ),
    timerForClose_( new QTimer )
{
    timerForClose_->setInterval( 30 * 1000 );

    connect( tcpSocket_.data(), &QTcpSocket::readyRead, [ this ]()
    {
        if ( this->timerForClose_->isActive() )
        {
            timerForClose_->stop();
        }

        this->buffer_.append( this->tcpSocket_->readAll() );

//        qDebug() << this->buffer_;

        this->inspectionBufferSetup1();

        timerForClose_->start();
    } );

    connect( tcpSocket_.data(), &QTcpSocket::bytesWritten, [ this ](const auto &bytes)
    {
        this->waitWrittenByteCount_ -= bytes;

        if ( this->waitWrittenByteCount_ == 0 )
        {
            this->deleteLater();
            return;
        }

        if ( this->timerForClose_->isActive() )
        {
            timerForClose_->stop();
        }

        timerForClose_->start();
    } );

    connect( timerForClose_.data(), &QTimer::timeout, this, &QObject::deleteLater );
}

Session::~Session()
{
    if ( !tcpSocket_.isNull() )
    {
        delete tcpSocket_.data();
    }
}

void Session::replyText(const QString &replyData)
{
    if ( QThread::currentThread() != this->thread() )
    {
        QMetaObject::invokeMethod( this, "replyText", Qt::QueuedConnection, Q_ARG( QString, replyData ) );
        return;
    }

    if ( tcpSocket_.isNull() )
    {
        qDebug() << "JQHttpServer::Session::replyText: error1";
        this->deleteLater();
        return;
    }

    const auto &&data = replyTextFormat.arg( QString::number( replyData.size() ), replyData ).toUtf8();
    waitWrittenByteCount_ = data.size();
    tcpSocket_->write( data );
}

void Session::inspectionBufferSetup1()
{
    if ( !headerAcceptedFinish_ )
    {
        forever
        {
            static QByteArray splitFlag( "\r\n" );

            auto splitFlagIndex = buffer_.indexOf( splitFlag );
            if ( splitFlagIndex == -1 ) { return; }

            if ( requestMethodToken_.isEmpty() && ( splitFlagIndex == 0 ) )
            {
                qDebug() << "JQHttpServer::Session::inspectionBuffer: error1";
                this->deleteLater();
                return;
            }

            if ( requestMethodToken_.isEmpty() )
            {
                auto requestLineDatas = buffer_.mid( 0, splitFlagIndex ).split( ' ' );
                buffer_.remove( 0, splitFlagIndex + 2 );

                if ( requestLineDatas.size() != 3 )
                {
                    qDebug() << "JQHttpServer::Session::inspectionBuffer: error2";
                    this->deleteLater();
                    return;
                }

                requestMethodToken_ = requestLineDatas.at( 0 );
                requestUrl_ = requestLineDatas.at( 1 );
                requestCrlf_ = requestLineDatas.at( 2 );

                if ( ( requestMethodToken_ != "GET" ) &&
                     ( requestMethodToken_ != "POST" ) )
                {
                    qDebug() << "JQHttpServer::Session::inspectionBuffer: error3";
                    this->deleteLater();
                    return;
                }

//                qDebug() << "requestMethodToken:" << requestMethodToken_;
//                qDebug() << "requestUrl:" << requestUrl_;
//                qDebug() << "requestCrlf:" << requestCrlf_;
            }
            else if ( splitFlagIndex == 0 )
            {
                buffer_.remove( 0, 2 );

                headerAcceptedFinish_ = true;

                if ( ( requestMethodToken_.toUpper() == "GET" ) ||
                   ( ( requestMethodToken_.toUpper() == "POST" ) && !buffer_.isEmpty() ) )
                {
                    this->inspectionBufferSetup2();
                }
            }
            else
            {
                auto index = buffer_.indexOf( ':' );

                if ( index <= 0 )
                {
                    qDebug() << "JQHttpServer::Session::inspectionBuffer: error4";
                    this->deleteLater();
                    return;
                }

                auto headerData = buffer_.mid( 0, splitFlagIndex );
                buffer_.remove( 0, splitFlagIndex + 2 );

                const auto &&key = headerData.mid( 0, index );
                auto value = headerData.mid( index + 1 );

                if ( value.startsWith( ' ' ) )
                {
                    value.remove( 0, 1 );
                }

                headersData_[ key ] = value;

//                qDebug() << "headerData:" << key << value;
            }
        }
    }
    else
    {
        this->inspectionBufferSetup2();
    }
}

void Session::inspectionBufferSetup2()
{
    requestRawData_ = buffer_;
    buffer_.clear();

//    qDebug() << "requestRawData:" << requestRawData_;

    if ( !handleAcceptedCallback_ )
    {
        qDebug() << "JQHttpServer::Session::inspectionBuffer: error4";
        this->deleteLater();
        return;
    }
    handleAcceptedCallback_( this );
}

// Manage
Manage::Manage()
{
    handleThreadPool_.reset( new QThreadPool );
    tcpSocketThreadPool_.reset( new QThreadPool );

    handleThreadPool_->setMaxThreadCount( 2 );
    tcpSocketThreadPool_->setMaxThreadCount( 1 );
}

Manage::~Manage()
{
    if ( this->isListening() )
    {
        this->close();
    }

    this->stopHandleThread();
}

bool Manage::listen(const QHostAddress &address, const quint16 &port)
{
    if ( QThread::currentThread() != this->thread() )
    {
        qDebug() << "JQHttpServer::Manage::listen: error: listen from other thread";
        return false;
    }

    if ( !tcpServer_.isNull() )
    {
        qDebug() << "JQHttpServer::Manage::close: error: already listen";
        return false;
    }

    return this->startTcpSocketThread( address, port );
}

void Manage::close()
{
    if ( tcpServer_.isNull() )
    {
        qDebug() << "JQHttpServer::Manage::close: error: not listen";
        return;
    }

    emit readyToClose();

    if ( tcpSocketThreadPool_->activeThreadCount() )
    {
        this->stopTcpSocketThread();
    }
}

bool Manage::startTcpSocketThread(const QHostAddress &address, const quint16 &port)
{
    QSemaphore semaphore;

    QtConcurrent::run( tcpSocketThreadPool_.data(), [ &semaphore, this, address, port ]()
    {
        QEventLoop eventLoop;
        QObject::connect( this, &Manage::readyToClose, &eventLoop, &QEventLoop::quit );

        QTcpServer tcpServer;
        QObject::connect( &tcpServer, &QTcpServer::newConnection, [ &tcpServer, this, address, port ]()
        {
            this->newSession( new Session( tcpServer.nextPendingConnection() ) );
        } );

        if ( !tcpServer.listen( address, port ) )
        {
            semaphore.release( 1 );
            return;
        }

        this->mutex_.lock();
        this->tcpServer_ = &tcpServer;
        this->mutex_.unlock();

        semaphore.release( 1 );

        eventLoop.exec();

        this->mutex_.lock();
        this->tcpServer_.clear();
        this->mutex_.unlock();

        tcpServer.close();
    } );

    semaphore.acquire( 1 );

    return !tcpServer_.isNull();
}

void Manage::stopHandleThread()
{
    handleThreadPool_->waitForDone();
}

void Manage::stopTcpSocketThread()
{
    tcpSocketThreadPool_->waitForDone();
}

void Manage::newSession(const QPointer< Session > &session)
{
//    qDebug() << "newConnection:" << session.data();

    session->setHandleAcceptedCallback( [ this ](const auto &session){ this->handleAccepted( session ); } );

    connect( session.data(), &QObject::destroyed, [ this, session = session.data() ]()
    {
//        qDebug() << "disConnection:" << session;

        this->mutex_.lock();
        this->availableSessions_.remove( session );
        this->mutex_.unlock();
    } );
    availableSessions_.insert( session.data() );
}

void Manage::handleAccepted(const QPointer<Session> &session)
{
    QtConcurrent::run( handleThreadPool_.data(), [ this, session ]()
    {
        if ( !this->httpAcceptedCallback_ )
        {
            qDebug() << "JQHttpServer::Manage::handleAccepted: error, httpAcceptedCallback_ is nullptr";
            return;
        }

        this->httpAcceptedCallback_( session );
    } );
}
