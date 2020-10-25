﻿/*
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

#include "JQNet.h"

// Qt lib import
#include <QEventLoop>
#include <QTimer>
#include <QUrl>
#include <QHostInfo>
#include <QTcpSocket>
#include <QCoreApplication>

// JQLibrary lib import
#ifdef JQFOUNDATION_LIB
#   include "JQFoundation.h"
#endif

QNetworkAddressEntry JQNet::getFirstNetworkAddressEntry()
{
    auto list = JQNet::getNetworkAddressEntryAndInterface();
    if ( list.isEmpty() ) { return { }; }

    return list.first().first;
}

QPair< QNetworkAddressEntry, QNetworkInterface > JQNet::getFirstNetworkAddressEntryAndInterface(const bool &ridVm)
{
    auto list = JQNet::getNetworkAddressEntryAndInterface( ridVm );
    if ( list.isEmpty() ) { return { }; }

    return list.first();
}

QList< QPair< QNetworkAddressEntry, QNetworkInterface > > JQNet::getNetworkAddressEntryAndInterface(const bool &ridVm)
{
    QList< QPair< QNetworkAddressEntry, QNetworkInterface > > result;

    for ( const auto &interface: static_cast< const QList< QNetworkInterface > >( QNetworkInterface::allInterfaces() ) )
    {
        if ( interface.flags() != ( QNetworkInterface::IsUp |
                                    QNetworkInterface::IsRunning |
                                    QNetworkInterface::CanBroadcast |
                                    QNetworkInterface::CanMulticast ) ) { continue; }

        if ( ridVm && interface.humanReadableName().startsWith( "vm" ) ) { continue; }

        for ( const auto &entry: static_cast< QList<QNetworkAddressEntry> >( interface.addressEntries() ) )
        {
            if ( entry.ip().toIPv4Address() )
            {
                result.push_back( { entry, interface } );
            }
        }
    }

    return result;
}

QString JQNet::getHostName()
{
#if ( defined Q_OS_MAC )
    return QHostInfo::localHostName().replace( ".local", "" );
#else
    return QHostInfo::localHostName();
#endif
}

bool JQNet::tcpReachable(const QString &hostName, const quint16 &port, const int &timeout)
{
    QTcpSocket socket;

    socket.connectToHost( hostName, port );
    socket.waitForConnected( timeout );

    return socket.state() == QAbstractSocket::ConnectedState;
}

#ifdef JQFOUNDATION_LIB
bool JQNet::pingReachable(const QString &address, const int &timeout)
{
    QPair< int, QByteArray > pingResult = { -1, { } };

#if ( defined Q_OS_MAC )
    pingResult = JQFoundation::startProcessAndReadOutput( "ping", { "-c1", QString( "-W%1" ).arg( timeout ), address } );
#elif ( defined Q_OS_WIN )
    pingResult = JQFoundation::startProcessAndReadOutput( "ping", { "-n", "1", "-w", QString::number( timeout ), address } );
#else
    Q_UNUSED( timeout )
#endif

    return ( pingResult.first == 0 ) && ( pingResult.second.size() > 20 ) && ( pingResult.second.count( address.toUtf8() ) > 1 );
}
#endif

// HTTP
bool JQNet::HTTP::get(
        const QNetworkRequest &request,
        QByteArray &receiveBuffer, const int &timeout
    )
{
    receiveBuffer.clear();

    QEventLoop eventLoop;
    auto reply = manage_.get( request );
    bool isFail = false;

    QObject::connect( qApp, &QCoreApplication::aboutToQuit, &eventLoop, &QEventLoop::quit );

    this->handle(
        reply,
        timeout,
        [ & ](const QList< QNetworkReply::RawHeaderPair > &, const QByteArray &data)
        {
            receiveBuffer = data;
            eventLoop.exit( 1 );
        },
        [ & ](const QList< QNetworkReply::RawHeaderPair > &, const QNetworkReply::NetworkError &, const QByteArray &data)
        {
            receiveBuffer = data;
            eventLoop.exit( 0 );
        },
        [ & ]()
        {
            isFail = true;
            eventLoop.exit( 0 );
        }
    );

    return eventLoop.exec() && !isFail;
}

void JQNet::HTTP::get(
        const QNetworkRequest &request,
        const std::function<void (const QList< QNetworkReply::RawHeaderPair > &, const QByteArray &)> &onFinished,
        const std::function<void (const QList< QNetworkReply::RawHeaderPair > &, const QNetworkReply::NetworkError &, const QByteArray &)> &onError,
        const int &timeout
    )
{
    auto reply = manage_.get( request );

    this->handle(
        reply,
        timeout,
        onFinished,
        onError,
        [ onError ]()
        {
            onError( { }, QNetworkReply::TimeoutError, { } );
        }
    );
}

bool JQNet::HTTP::deleteResource(
        const QNetworkRequest &request,
        QByteArray &receiveBuffer,
        const int &timeout
    )
{
    receiveBuffer.clear();

    QEventLoop eventLoop;
    auto reply = manage_.deleteResource( request );
    bool isFail = false;

    QObject::connect( qApp, &QCoreApplication::aboutToQuit, &eventLoop, &QEventLoop::quit );

    this->handle(
        reply,
        timeout,
        [ & ](const QList< QNetworkReply::RawHeaderPair > &, const QByteArray &data)
        {
            receiveBuffer = data;
            eventLoop.exit( 1 );
        },
        [ & ](const QList< QNetworkReply::RawHeaderPair > &, const QNetworkReply::NetworkError &, const QByteArray &data)
        {
            receiveBuffer = data;
            eventLoop.exit( 0 );
        },
        [ & ]()
        {
            isFail = true;
            eventLoop.exit( 0 );
        }
    );

    return eventLoop.exec() && !isFail;
}

void JQNet::HTTP::deleteResource(
        const QNetworkRequest &request,
        const std::function<void (const QList< QNetworkReply::RawHeaderPair > &, const QByteArray &)> &onFinished,
        const std::function<void (const QList< QNetworkReply::RawHeaderPair > &, const QNetworkReply::NetworkError &, const QByteArray &)> &onError,
        const int &timeout
    )
{
    auto reply = manage_.deleteResource( request );

    this->handle(
        reply,
        timeout,
        onFinished,
        onError,
        [ onError ]()
        {
            onError( { }, QNetworkReply::TimeoutError, { } );
        }
    );
}

bool JQNet::HTTP::post(
        const QNetworkRequest &request,
        const QByteArray &body,
        QList< QNetworkReply::RawHeaderPair > &receiveRawHeaderPairs,
        QByteArray &receiveBuffer,
        const int &timeout
    )
{
    receiveBuffer.clear();

    QEventLoop eventLoop;
    auto reply = manage_.post( request, body );
    bool isFail = false;

    QObject::connect( qApp, &QCoreApplication::aboutToQuit, &eventLoop, &QEventLoop::quit );

    this->handle(
        reply,
        timeout,
        [ &receiveRawHeaderPairs, &receiveBuffer, &eventLoop ](const QList< QNetworkReply::RawHeaderPair > &rawHeaderPairs, const QByteArray &data)
        {
            receiveRawHeaderPairs = rawHeaderPairs;
            receiveBuffer = data;
            eventLoop.exit( true );
        },
        [ &receiveRawHeaderPairs, &receiveBuffer, &eventLoop ](const QList< QNetworkReply::RawHeaderPair > &rawHeaderPairs, const QNetworkReply::NetworkError &, const QByteArray &data)
        {
            receiveRawHeaderPairs = rawHeaderPairs;
            receiveBuffer = data;
            eventLoop.exit( false );
        },
        [ &isFail, &eventLoop ]()
        {
            isFail = true;
            eventLoop.exit( false );
        }
    );

    return eventLoop.exec() && !isFail;
}

bool JQNet::HTTP::post(
        const QNetworkRequest &request,
        const QSharedPointer< QHttpMultiPart > &multiPart,
        QByteArray &receiveBuffer,
        const int &timeout
    )
{
    receiveBuffer.clear();

    QEventLoop eventLoop;
    auto reply = manage_.post( request, multiPart.data() );
    bool isFail = false;

    QObject::connect( qApp, &QCoreApplication::aboutToQuit, &eventLoop, &QEventLoop::quit );

    this->handle(
        reply,
        timeout,
        [ &receiveBuffer, &eventLoop ](const QList< QNetworkReply::RawHeaderPair > &, const QByteArray &data)
        {
            receiveBuffer = data;
            eventLoop.exit( true );
        },
        [ &receiveBuffer, &eventLoop ](const QList< QNetworkReply::RawHeaderPair > &, const QNetworkReply::NetworkError &, const QByteArray &data)
        {
            receiveBuffer = data;
            eventLoop.exit( false );
        },
        [ &isFail, &eventLoop ]()
        {
            isFail = true;
            eventLoop.exit( false );
        }
    );

    return eventLoop.exec() && !isFail;
}

void JQNet::HTTP::post(
        const QNetworkRequest &request,
        const QByteArray &body,
        const std::function<void (const QList< QNetworkReply::RawHeaderPair > &, const QByteArray &)> &onFinished,
        const std::function<void (const QList< QNetworkReply::RawHeaderPair > &, const QNetworkReply::NetworkError &, const QByteArray &)> &onError,
        const int &timeout
    )
{
    auto reply = manage_.post( request, body );

    this->handle(
        reply,
        timeout,
        onFinished,
        onError,
        [ onError ]()
        {
            onError( { }, QNetworkReply::TimeoutError, { } );
        }
    );
}

bool JQNet::HTTP::put(
        const QNetworkRequest &request,
        const QByteArray &body,
        QByteArray &receiveBuffer,
        const int &timeout
    )
{
    receiveBuffer.clear();

    QEventLoop eventLoop;
    auto reply = manage_.put( request, body );
    bool isFail = false;

    QObject::connect( qApp, &QCoreApplication::aboutToQuit, &eventLoop, &QEventLoop::quit );

    this->handle(
        reply,
        timeout,
        [ &receiveBuffer, &eventLoop ](const QList< QNetworkReply::RawHeaderPair > &, const QByteArray &data)
        {
            receiveBuffer = data;
            eventLoop.exit( true );
        },
        [ &receiveBuffer, &eventLoop ](const QList< QNetworkReply::RawHeaderPair > &, const QNetworkReply::NetworkError &, const QByteArray &data)
        {
            receiveBuffer = data;
            eventLoop.exit( false );
        },
        [ &isFail, &eventLoop ]()
        {
            isFail = true;
            eventLoop.exit( false );
        }
    );

    return eventLoop.exec() && !isFail;
}

bool JQNet::HTTP::put(
        const QNetworkRequest &request,
        const QSharedPointer< QHttpMultiPart > &multiPart,
        QByteArray &receiveBuffer,
        const int &timeout
    )
{
    receiveBuffer.clear();

    QEventLoop eventLoop;
    auto reply = manage_.put( request, multiPart.data() );
    bool isFail = false;

    QObject::connect( qApp, &QCoreApplication::aboutToQuit, &eventLoop, &QEventLoop::quit );

    this->handle(
        reply,
        timeout,
        [ &receiveBuffer, &eventLoop ](const QList< QNetworkReply::RawHeaderPair > &, const QByteArray &data)
        {
            receiveBuffer = data;
            eventLoop.exit( true );
        },
        [ &receiveBuffer, &eventLoop ](const QList< QNetworkReply::RawHeaderPair > &, const QNetworkReply::NetworkError & /*e*/, const QByteArray &data)
        {
            receiveBuffer = data;
            eventLoop.exit( false );
        },
        [ &isFail, &eventLoop ]()
        {
            isFail = true;
            eventLoop.exit( false );
        }
    );

    return eventLoop.exec() && !isFail;
}

void JQNet::HTTP::put(
        const QNetworkRequest &request,
        const QByteArray &body,
        const std::function<void (const QList< QNetworkReply::RawHeaderPair > &, const QByteArray &)> &onFinished,
        const std::function<void (const QList< QNetworkReply::RawHeaderPair > &, const QNetworkReply::NetworkError &, const QByteArray &)> &onError,
        const int &timeout
    )
{
    auto reply = manage_.put( request, body );

    this->handle(
        reply,
        timeout,
        onFinished,
        onError,
        [ onError ]()
        {
            onError( { }, QNetworkReply::TimeoutError, { } );
        }
    );
}

#if !( defined Q_OS_LINUX ) && ( QT_VERSION >= QT_VERSION_CHECK( 5, 9, 0 ) )
bool JQNet::HTTP::patch(
        const QNetworkRequest &request,
        const QByteArray &body,
        QByteArray &receiveBuffer,
        const int &timeout
    )
{
    receiveBuffer.clear();

    QEventLoop eventLoop;
    auto reply = manage_.sendCustomRequest( request, "PATCH", body );
    bool isFail = false;

    QObject::connect( qApp, &QCoreApplication::aboutToQuit, &eventLoop, &QEventLoop::quit );

    this->handle(
        reply,
        timeout,
        [ &receiveBuffer, &eventLoop ](const QList< QNetworkReply::RawHeaderPair > &, const QByteArray &data)
        {
            receiveBuffer = data;
            eventLoop.exit( true );
        },
        [ &receiveBuffer, &eventLoop ](const QList< QNetworkReply::RawHeaderPair > &, const QNetworkReply::NetworkError &, const QByteArray &data)
        {
            receiveBuffer = data;
            eventLoop.exit( false );
        },
        [ &isFail, &eventLoop ]()
        {
            isFail = true;
            eventLoop.exit( false );
        }
    );

    return eventLoop.exec() && !isFail;
}

void JQNet::HTTP::patch(
        const QNetworkRequest &request,
        const QByteArray &body,
        const std::function<void (const QList< QNetworkReply::RawHeaderPair > &, const QByteArray &)> &onFinished,
        const std::function<void (const QList< QNetworkReply::RawHeaderPair > &, const QNetworkReply::NetworkError &, const QByteArray &)> &onError,
        const int &timeout
    )
{
    auto reply = manage_.sendCustomRequest( request, "PATCH", body );

    this->handle(
        reply,
        timeout,
        onFinished,
        onError,
        [ onError ]()
        {
            onError( { }, QNetworkReply::TimeoutError, { } );
        }
    );
}
#endif

QPair< bool, QByteArray > JQNet::HTTP::get(const QString &url, const int &timeout)
{
    QNetworkRequest networkRequest( ( QUrl( url ) ) );
    QByteArray receiveBuffer;

    const auto &&isSucceed = HTTP().get( networkRequest, receiveBuffer, timeout );

    return { isSucceed, receiveBuffer };
}

QPair< bool, QByteArray > JQNet::HTTP::get(const QNetworkRequest &request, const int &timeout)
{
    QByteArray receiveBuffer;
    HTTP http;

    const auto &&isSucceed = http.get( request, receiveBuffer, timeout );

    return { isSucceed, receiveBuffer };
}

QPair< bool, QByteArray > JQNet::HTTP::deleteResource(const QString &url, const int &timeout)
{
    QNetworkRequest networkRequest( ( QUrl( url ) ) );
    QByteArray receiveBuffer;

    const auto &&isSucceed = HTTP().deleteResource( networkRequest, receiveBuffer, timeout );

    return { isSucceed, receiveBuffer };
}

QPair< bool, QByteArray > JQNet::HTTP::deleteResource(const QNetworkRequest &request, const int &timeout)
{
    QByteArray receiveBuffer;
    HTTP http;

    const auto &&isSucceed = http.deleteResource( request, receiveBuffer, timeout );

    return { isSucceed, receiveBuffer };
}

QPair< bool, QByteArray > JQNet::HTTP::post(const QString &url, const QByteArray &body, const int &timeout)
{
    QNetworkRequest networkRequest( ( QUrl( url ) ) );
    QList< QNetworkReply::RawHeaderPair > rawHeaderPairs;
    QByteArray receiveBuffer;

    networkRequest.setRawHeader( "Content-Type", "application/json;charset=UTF-8" );

    const auto &&isSucceed = HTTP().post( networkRequest, body, rawHeaderPairs, receiveBuffer, timeout );

    return { isSucceed, receiveBuffer };
}

QPair< bool, QByteArray > JQNet::HTTP::post(const QNetworkRequest &request, const QByteArray &body, const int &timeout)
{
    QByteArray receiveBuffer;
    QList< QNetworkReply::RawHeaderPair > rawHeaderPairs;
    HTTP http;

    const auto &&isSucceed = http.post( request, body, rawHeaderPairs, receiveBuffer, timeout );

    return { isSucceed, receiveBuffer };
}

QPair< bool, QPair< QList< QNetworkReply::RawHeaderPair >, QByteArray > > JQNet::HTTP::post2(const QNetworkRequest &request, const QByteArray &body, const int &timeout)
{
    QByteArray receiveBuffer;
    QList< QNetworkReply::RawHeaderPair > rawHeaderPairs;
    HTTP http;

    const auto &&isSucceed = http.post( request, body, rawHeaderPairs, receiveBuffer, timeout );

    return { isSucceed, { rawHeaderPairs, receiveBuffer } };
}

QPair< bool, QByteArray > JQNet::HTTP::post(const QNetworkRequest &request, const QSharedPointer<QHttpMultiPart> &multiPart, const int &timeout)
{
    QByteArray receiveBuffer;
    HTTP http;

    const auto &&isSucceed = http.post( request, multiPart, receiveBuffer, timeout );

    return { isSucceed, receiveBuffer };
}

QPair< bool, QByteArray > JQNet::HTTP::put(const QString &url, const QByteArray &body, const int &timeout)
{
    QNetworkRequest networkRequest( ( QUrl( url ) ) );
    QByteArray receiveBuffer;

    networkRequest.setRawHeader( "Content-Type", "application/json;charset=UTF-8" );

    const auto &&isSucceed = HTTP().put( networkRequest, body, receiveBuffer, timeout );

    return { isSucceed, receiveBuffer };
}

QPair< bool, QByteArray > JQNet::HTTP::put(const QNetworkRequest &request, const QByteArray &body, const int &timeout)
{
    QByteArray receiveBuffer;
    HTTP http;

    const auto &&isSucceed = http.put( request, body, receiveBuffer, timeout );

    return { isSucceed, receiveBuffer };
}

QPair< bool, QByteArray > JQNet::HTTP::put(const QNetworkRequest &request, const QSharedPointer< QHttpMultiPart > &multiPart, const int &timeout)
{
    QByteArray receiveBuffer;
    HTTP http;

    const auto &&isSucceed = http.put( request, multiPart, receiveBuffer, timeout );

    return { isSucceed, receiveBuffer };
}

#if !( defined Q_OS_LINUX ) && ( QT_VERSION >= QT_VERSION_CHECK( 5, 9, 0 ) )
QPair< bool, QByteArray > JQNet::HTTP::patch(const QString &url, const QByteArray &body, const int &timeout)
{
    QNetworkRequest networkRequest( ( QUrl( url ) ) );
    QByteArray receiveBuffer;

    networkRequest.setRawHeader( "Content-Type", "application/json;charset=UTF-8" );

    const auto &&isSucceed = HTTP().patch( networkRequest, body, receiveBuffer, timeout );

    return { isSucceed, receiveBuffer };
}

QPair< bool, QByteArray > JQNet::HTTP::patch(const QNetworkRequest &request, const QByteArray &body, const int &timeout)
{
    QByteArray receiveBuffer;
    HTTP http;

    const auto &&isSucceed = http.patch( request, body, receiveBuffer, timeout );

    return { isSucceed, receiveBuffer };
}
#endif

void JQNet::HTTP::handle(
        QNetworkReply *reply,
        const int &timeout,
        const std::function<void (const QList< QNetworkReply::RawHeaderPair > &, const QByteArray &)> &onFinished,
        const std::function<void (const QList< QNetworkReply::RawHeaderPair > &, const QNetworkReply::NetworkError &, const QByteArray &data)> &onError,
        const std::function<void ()> &onTimeout
    )
{
    QSharedPointer< bool > isCalled( new bool( false ) );

    QTimer *timer = nullptr;
    if ( timeout )
    {
        timer = new QTimer;
        timer->setSingleShot(true);

        QObject::connect( timer, &QTimer::timeout, [ timer, onTimeout, isCalled ]()
        {
            if ( *isCalled ) { return; }
            *isCalled = true;

            onTimeout();
            timer->deleteLater();
        } );
        timer->start( timeout );
    }

    QObject::connect( reply, &QNetworkReply::finished, [ reply, timer, onFinished, isCalled ]()
    {
        if ( *isCalled ) { return; }
        *isCalled = true;

        if ( timer )
        {
            timer->deleteLater();
        }

        const auto &&acceptedData = reply->readAll();
        const auto &rawHeaderPairs = reply->rawHeaderPairs();

        onFinished( rawHeaderPairs, acceptedData );
    } );

#ifndef QT_NO_SSL
    if ( reply->url().toString().toLower().startsWith( "https" ) )
    {
        QObject::connect( reply, static_cast< void( QNetworkReply::* )( const QList< QSslError > & ) >( &QNetworkReply::sslErrors ), [ reply ](const QList< QSslError > & /*errors*/)
        {
//            qDebug() << "HTTP::handle: ignoreSslErrors:" << errors;
            reply->ignoreSslErrors();
        } );
    }
#endif

    QObject::connect( reply, static_cast< void( QNetworkReply::* )( QNetworkReply::NetworkError ) >( &QNetworkReply::error ), [ reply, timer, onError, isCalled ](const QNetworkReply::NetworkError &code)
    {
        if ( *isCalled ) { return; }
        *isCalled = true;

        if ( timer )
        {
            timer->deleteLater();
        }
        const auto &&acceptedData = reply->readAll();
        const auto &rawHeaderPairs = reply->rawHeaderPairs();

        onError( rawHeaderPairs, code, acceptedData );
    } );
}
