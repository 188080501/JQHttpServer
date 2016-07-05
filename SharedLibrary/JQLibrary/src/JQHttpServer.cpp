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
#include <QLocalServer>
#include <QLocalSocket>
#include <QtConcurrent>
#include <QJsonDocument>
#include <QJsonValue>

using namespace JQHttpServer;

static QString replyTextFormat(
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %1\r\n"
        "Content-Length: %2\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n"
        "%3"
    );

// Session
Session::Session(const QPointer<QIODevice> &tcpSocket):
    ioDevice_( tcpSocket ),
    timerForClose_( new QTimer )
{
    timerForClose_->setInterval( 30 * 1000 );

    connect( ioDevice_.data(), &QIODevice::readyRead, [ this ]()
    {
        if ( this->timerForClose_->isActive() )
        {
            timerForClose_->stop();
        }

        this->buffer_.append( this->ioDevice_->readAll() );

//        qDebug() << this->buffer_;

        this->inspectionBufferSetup1();

        timerForClose_->start();
    } );

    connect( ioDevice_.data(), &QIODevice::bytesWritten, [ this ](const auto &bytes)
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
    if ( !ioDevice_.isNull() )
    {
        delete ioDevice_.data();
    }
}

void Session::replyText(const QString &replyData)
{
    if ( QThread::currentThread() != this->thread() )
    {
        QMetaObject::invokeMethod( this, "replyText", Qt::QueuedConnection, Q_ARG( QString, replyData ) );
        return;
    }

    if ( ioDevice_.isNull() )
    {
        qDebug() << "JQHttpServer::Session::replyText: error1";
        this->deleteLater();
        return;
    }

    const auto &&data = replyTextFormat.arg( "text;charset=UTF-8", QString::number( replyData.toUtf8().size() ), replyData ).toUtf8();

    waitWrittenByteCount_ = data.size();
    ioDevice_->write( data );
}

void Session::replyJsonObject(const QJsonObject &jsonObject)
{
    if ( QThread::currentThread() != this->thread() )
    {
        QMetaObject::invokeMethod( this, "replyJsonObject", Qt::QueuedConnection, Q_ARG( QJsonObject, jsonObject ) );
        return;
    }

    if ( ioDevice_.isNull() )
    {
        qDebug() << "JQHttpServer::Session::replyText: error1";
        this->deleteLater();
        return;
    }

    const auto &&data = QJsonDocument( jsonObject ).toJson();
    const auto &&data2 = replyTextFormat.arg( "application/json;charset=UTF-8", QString::number( data.size() ), QString( data ) ).toUtf8();

    waitWrittenByteCount_ = data2.size();
    ioDevice_->write( data2 );
}

void Session::replyJsonArray(const QJsonArray &jsonArray)
{
    if ( QThread::currentThread() != this->thread() )
    {
        QMetaObject::invokeMethod( this, "replyJsonArray", Qt::QueuedConnection, Q_ARG( QJsonArray, jsonArray ) );
        return;
    }

    if ( ioDevice_.isNull() )
    {
        qDebug() << "JQHttpServer::Session::replyText: error1";
        this->deleteLater();
        return;
    }

    const auto &&data = QJsonDocument( jsonArray ).toJson();
    const auto &&data2 = replyTextFormat.arg( "application/json;charset=UTF-8", QString::number( data.size() ), QString( data ) ).toUtf8();

    waitWrittenByteCount_ = data2.size();
    ioDevice_->write( data2 );
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

// AbstractManage
AbstractManage::AbstractManage()
{
    handleThreadPool_.reset( new QThreadPool );
    serverThreadPool_.reset( new QThreadPool );

    handleThreadPool_->setMaxThreadCount( 2 );
    serverThreadPool_->setMaxThreadCount( 1 );
}

AbstractManage::~AbstractManage()
{
    this->stopHandleThread();
}

bool AbstractManage::begin()
{
    if ( QThread::currentThread() != this->thread() )
    {
        qDebug() << "JQHttpServer::Manage::listen: error: listen from other thread";
        return false;
    }

    if ( this->isRunning() )
    {
        qDebug() << "JQHttpServer::Manage::close: error: already running";
        return false;
    }

    return this->startServerThread();
}

void AbstractManage::close()
{
    if ( !this->isRunning() )
    {
        qDebug() << "JQHttpServer::Manage::close: error: not running";
        return;
    }

    emit readyToClose();

    if ( serverThreadPool_->activeThreadCount() )
    {
        this->stopServerThread();
    }
}

bool AbstractManage::startServerThread()
{
    QSemaphore semaphore;

    QtConcurrent::run( serverThreadPool_.data(), [ &semaphore, this ]()
    {
        QEventLoop eventLoop;
        QObject::connect( this, &AbstractManage::readyToClose, &eventLoop, &QEventLoop::quit );

        if ( !this->onStart() )
        {
            semaphore.release( 1 );
        }

        semaphore.release( 1 );

        eventLoop.exec();

        this->onFinish();
    } );

    semaphore.acquire( 1 );

    return this->isRunning();
}

void AbstractManage::stopHandleThread()
{
    handleThreadPool_->waitForDone();
}

void AbstractManage::stopServerThread()
{
    serverThreadPool_->waitForDone();
}

void AbstractManage::newSession(const QPointer< Session > &session)
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

void AbstractManage::handleAccepted(const QPointer<Session> &session)
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

// TcpServerManage
TcpServerManage::~TcpServerManage()
{
    if ( this->isRunning() )
    {
        this->close();
    }
}

bool TcpServerManage::listen(const QHostAddress &address, const quint16 &port)
{
    listenAddress_ = address;
    listenPort_ = port;

    return this->begin();
}

bool TcpServerManage::isRunning()
{
    return !tcpServer_.isNull();
}

bool TcpServerManage::onStart()
{
    mutex_.lock();

    tcpServer_ = new QTcpServer;

    mutex_.unlock();

    QObject::connect( tcpServer_.data(), &QTcpServer::newConnection, [ this ]()
    {
        this->newSession( new Session( this->tcpServer_->nextPendingConnection() ) );
    } );

    if ( !tcpServer_->listen( listenAddress_, listenPort_ ) )
    {
        mutex_.lock();

        delete tcpServer_.data();
        tcpServer_.clear();

        mutex_.unlock();

        return false;
    }

    return true;
}

void TcpServerManage::onFinish()
{
    this->mutex_.lock();

    tcpServer_->close();
    delete tcpServer_.data();
    tcpServer_.clear();

    this->mutex_.unlock();
}

// LocalServerManage
LocalServerManage::~LocalServerManage()
{
    if ( this->isRunning() )
    {
        this->close();
    }
}

bool LocalServerManage::listen(const QString &name)
{
    listenName_ = name;

    return this->begin();
}

bool LocalServerManage::isRunning()
{
    return !localServer_.isNull();
}

bool LocalServerManage::onStart()
{
    mutex_.lock();

    localServer_ = new QLocalServer;

    mutex_.unlock();

    QObject::connect( localServer_.data(), &QLocalServer::newConnection, [ this ]()
    {
        this->newSession( new Session( this->localServer_->nextPendingConnection() ) );
    } );

    if ( !localServer_->listen( listenName_ ) )
    {
        mutex_.lock();

        delete localServer_.data();
        localServer_.clear();

        mutex_.unlock();

        return false;
    }

    return true;
}

void LocalServerManage::onFinish()
{
    this->mutex_.lock();

    localServer_->close();
    delete localServer_.data();
    localServer_.clear();

    this->mutex_.unlock();
}
