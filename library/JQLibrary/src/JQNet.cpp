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

// JQLibrary lib import
#ifdef JQFOUNDATION_LIB
#   include "JQFoundation.h"
#endif

QNetworkAddressEntry JQNet::getNetworkAddressEntry()
{
    return getNetworkAddressEntryWithNetworkInterface().first;
}

QPair< QNetworkAddressEntry, QNetworkInterface > JQNet::getNetworkAddressEntryWithNetworkInterface(const bool &ridVm)
{
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
                return { entry, interface };
            }
        }
    }

    return { };
}

QString JQNet::getHostName()
{
#if ( defined Q_OS_MAC )
    return QHostInfo::localHostName().replace( ".local", "" );
#else
    return QHostInfo::localHostName();
#endif
}

#ifdef JQFOUNDATION_LIB
bool JQNet::pingIp(const QHostAddress &hostAddress)
{
//    qDebug() << "NetworkManage::pingIp:" << hostAddress.toString();

    QPair< int, QByteArray > pingResult = { -1, { } };

#ifdef Q_OS_MAC
    pingResult = JQFoundation::startProcessAndReadOutput( "ping", { "-c1", "-W300", hostAddress.toString() } );
#endif

#ifdef Q_OS_WIN
    pingResult = JQFoundation::startProcessAndReadOutput( "ping", { "-n", "1", "-w", "300", hostAddress.toString() } );
#endif

    return ( pingResult.first == 0 ) && ( pingResult.second.size() > 20 ) && ( pingResult.second.count( hostAddress.toString().toUtf8() ) > 1 );
}
#endif

// HTTP
bool JQNet::HTTP::get(
        const QNetworkRequest &request,
        QByteArray &target, const int &timeout
    )
{
    target.clear();

    QEventLoop eventLoop;
    auto reply = manage_.get( request );
    bool failFlag = false;

    this->handle(
        reply,
        timeout,
        [ & ](const QByteArray &data)
        {
            target = data;
            eventLoop.exit( 1 );
        },
        [ & ](const QNetworkReply::NetworkError &, const QByteArray &data)
        {
            target = data;
            eventLoop.exit( 0 );
        },
        [ & ]()
        {
            failFlag = true;
            eventLoop.exit( 0 );
        }
    );

    return eventLoop.exec() && !failFlag;
}

void JQNet::HTTP::get(
        const QNetworkRequest &request,
        const std::function<void (const QByteArray &)> &onFinished,
        const std::function<void (const QNetworkReply::NetworkError &, const QByteArray &)> &onError,
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
            onError( QNetworkReply::TimeoutError, { } );
        }
    );
}

bool JQNet::HTTP::deleteResource(
        const QNetworkRequest &request,
        QByteArray &target,
        const int &timeout
    )
{
    target.clear();

    QEventLoop eventLoop;
    auto reply = manage_.deleteResource( request );
    bool failFlag = false;

    this->handle(
        reply,
        timeout,
        [ & ](const QByteArray &data)
        {
            target = data;
            eventLoop.exit( 1 );
        },
        [ & ](const QNetworkReply::NetworkError &, const QByteArray &data)
        {
            target = data;
            eventLoop.exit( 0 );
        },
        [ & ]()
        {
            failFlag = true;
            eventLoop.exit( 0 );
        }
    );

    return eventLoop.exec() && !failFlag;
}

void JQNet::HTTP::deleteResource(
        const QNetworkRequest &request,
        const std::function<void (const QByteArray &)> &onFinished,
        const std::function<void (const QNetworkReply::NetworkError &, const QByteArray &)> &onError,
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
            onError( QNetworkReply::TimeoutError, { } );
        }
    );
}

bool JQNet::HTTP::post(
        const QNetworkRequest &request,
        const QByteArray &body,
        QByteArray &target,
        const int &timeout
    )
{
    target.clear();

    QEventLoop eventLoop;
    auto reply = manage_.post( request, body );
    bool failFlag = false;

    this->handle(
        reply,
        timeout,
        [ &target, &eventLoop ](const QByteArray &data)
        {
            target = data;
            eventLoop.exit( true );
        },
        [ &target, &eventLoop ](const QNetworkReply::NetworkError &, const QByteArray &data)
        {
            target = data;
            eventLoop.exit( false );
        },
        [ &failFlag, &eventLoop ]()
        {
            failFlag = true;
            eventLoop.exit( false );
        }
    );

    return eventLoop.exec() && !failFlag;
}

bool JQNet::HTTP::post(
        const QNetworkRequest &request,
        const QSharedPointer< QHttpMultiPart > &multiPart,
        QByteArray &target,
        const int &timeout
    )
{
    target.clear();

    QEventLoop eventLoop;
    auto reply = manage_.post( request, multiPart.data() );
    bool failFlag = false;

    this->handle(
        reply,
        timeout,
        [ &target, &eventLoop ](const QByteArray &data)
        {
            target = data;
            eventLoop.exit( true );
        },
        [ &target, &eventLoop ](const QNetworkReply::NetworkError &, const QByteArray &data)
        {
            target = data;
            eventLoop.exit( false );
        },
        [ &failFlag, &eventLoop ]()
        {
            failFlag = true;
            eventLoop.exit( false );
        }
    );

    return eventLoop.exec() && !failFlag;
}

void JQNet::HTTP::post(
        const QNetworkRequest &request,
        const QByteArray &body,
        const std::function<void (const QByteArray &)> &onFinished,
        const std::function<void (const QNetworkReply::NetworkError &, const QByteArray &)> &onError,
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
            onError( QNetworkReply::TimeoutError, { } );
        }
    );
}

bool JQNet::HTTP::put(
        const QNetworkRequest &request,
        const QByteArray &body,
        QByteArray &target,
        const int &timeout
    )
{
    target.clear();

    QEventLoop eventLoop;
    auto reply = manage_.put( request, body );
    bool failFlag = false;

    this->handle(
        reply,
        timeout,
        [ &target, &eventLoop ](const QByteArray &data)
        {
            target = data;
            eventLoop.exit( true );
        },
        [ &target, &eventLoop ](const QNetworkReply::NetworkError &, const QByteArray &data)
        {
            target = data;
            eventLoop.exit( false );
        },
        [ &failFlag, &eventLoop ]()
        {
            failFlag = true;
            eventLoop.exit( false );
        }
    );

    return eventLoop.exec() && !failFlag;
}

bool JQNet::HTTP::put(
        const QNetworkRequest &request,
        const QSharedPointer< QHttpMultiPart > &multiPart,
        QByteArray &target,
        const int &timeout
    )
{
    target.clear();

    QEventLoop eventLoop;
    auto reply = manage_.put( request, multiPart.data() );
    bool failFlag = false;

    this->handle(
        reply,
        timeout,
        [ &target, &eventLoop ](const QByteArray &data)
        {
            target = data;
            eventLoop.exit( true );
        },
        [ &target, &eventLoop ](const QNetworkReply::NetworkError &e, const QByteArray &data)
        {
            qDebug() << e;
            target = data;
            eventLoop.exit( false );
        },
        [ &failFlag, &eventLoop ]()
        {
            failFlag = true;
            eventLoop.exit( false );
        }
    );

    return eventLoop.exec() && !failFlag;
}

void JQNet::HTTP::put(
        const QNetworkRequest &request,
        const QByteArray &body,
        const std::function<void (const QByteArray &)> &onFinished,
        const std::function<void (const QNetworkReply::NetworkError &, const QByteArray &)> &onError,
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
            onError( QNetworkReply::TimeoutError, { } );
        }
    );
}

#if !( defined Q_OS_LINUX ) && ( QT_VERSION >= QT_VERSION_CHECK( 5, 9, 0 ) )
bool JQNet::HTTP::patch(
        const QNetworkRequest &request,
        const QByteArray &body,
        QByteArray &target,
        const int &timeout
    )
{
    target.clear();

    QEventLoop eventLoop;
    auto reply = manage_.sendCustomRequest( request, "PATCH", body );
    bool failFlag = false;

    this->handle(
        reply,
        timeout,
        [ &target, &eventLoop ](const QByteArray &data)
        {
            target = data;
            eventLoop.exit( true );
        },
        [ &target, &eventLoop ](const QNetworkReply::NetworkError &, const QByteArray &data)
        {
            target = data;
            eventLoop.exit( false );
        },
        [ &failFlag, &eventLoop ]()
        {
            failFlag = true;
            eventLoop.exit( false );
        }
    );

    return eventLoop.exec() && !failFlag;
}

void JQNet::HTTP::patch(
        const QNetworkRequest &request,
        const QByteArray &body,
        const std::function<void (const QByteArray &)> &onFinished,
        const std::function<void (const QNetworkReply::NetworkError &, const QByteArray &)> &onError,
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
            onError( QNetworkReply::TimeoutError, { } );
        }
    );
}
#endif

QPair< bool, QByteArray > JQNet::HTTP::get(const QString &url, const int &timeout)
{
    QNetworkRequest networkRequest( ( QUrl( url ) ) );
    QByteArray buf;

    const auto &&flag = HTTP().get( networkRequest, buf, timeout );

    return { flag, buf };
}

QPair< bool, QByteArray > JQNet::HTTP::get(const QNetworkRequest &request, const int &timeout)
{
    QByteArray buf;
    HTTP http;

    const auto &&flag = http.get( request, buf, timeout );

    return { flag, buf };
}

QPair< bool, QByteArray > JQNet::HTTP::deleteResource(const QString &url, const int &timeout)
{
    QNetworkRequest networkRequest( ( QUrl( url ) ) );
    QByteArray buf;

    const auto &&flag = HTTP().deleteResource( networkRequest, buf, timeout );

    return { flag, buf };
}

QPair< bool, QByteArray > JQNet::HTTP::deleteResource(const QNetworkRequest &request, const int &timeout)
{
    QByteArray buf;
    HTTP http;

    const auto &&flag = http.deleteResource( request, buf, timeout );

    return { flag, buf };
}

QPair< bool, QByteArray > JQNet::HTTP::post(const QString &url, const QByteArray &body, const int &timeout)
{
    QNetworkRequest networkRequest( ( QUrl( url ) ) );
    QByteArray buf;

    networkRequest.setRawHeader( "Content-Type", "application/x-www-form-urlencoded" );

    const auto &&flag = HTTP().post( networkRequest, body, buf, timeout );

    return { flag, buf };
}

QPair< bool, QByteArray > JQNet::HTTP::post(const QNetworkRequest &request, const QByteArray &body, const int &timeout)
{
    QByteArray buf;
    HTTP http;

    const auto &&flag = http.post( request, body, buf, timeout );

    return { flag, buf };
}

QPair< bool, QByteArray > JQNet::HTTP::post(const QNetworkRequest &request, const QSharedPointer<QHttpMultiPart> &multiPart, const int &timeout)
{
    QByteArray buf;
    HTTP http;

    const auto &&flag = http.post( request, multiPart, buf, timeout );

    return { flag, buf };
}

QPair< bool, QByteArray > JQNet::HTTP::put(const QString &url, const QByteArray &body, const int &timeout)
{
    QNetworkRequest networkRequest( ( QUrl( url ) ) );
    QByteArray buf;

    networkRequest.setRawHeader( "Content-Type", "application/x-www-form-urlencoded" );

    const auto &&flag = HTTP().put( networkRequest, body, buf, timeout );

    return { flag, buf };
}

QPair< bool, QByteArray > JQNet::HTTP::put(const QNetworkRequest &request, const QByteArray &body, const int &timeout)
{
    QByteArray buf;
    HTTP http;

    const auto &&flag = http.put( request, body, buf, timeout );

    return { flag, buf };
}

QPair< bool, QByteArray > JQNet::HTTP::put(const QNetworkRequest &request, const QSharedPointer< QHttpMultiPart > &multiPart, const int &timeout)
{
    QByteArray buf;
    HTTP http;

    const auto &&flag = http.put( request, multiPart, buf, timeout );

    return { flag, buf };
}

#if !( defined Q_OS_LINUX ) && ( QT_VERSION >= QT_VERSION_CHECK( 5, 9, 0 ) )
QPair< bool, QByteArray > JQNet::HTTP::patch(const QString &url, const QByteArray &body, const int &timeout)
{
    QNetworkRequest networkRequest( ( QUrl( url ) ) );
    QByteArray buf;

    networkRequest.setRawHeader( "Content-Type", "application/x-www-form-urlencoded" );

    const auto &&flag = HTTP().patch( networkRequest, body, buf, timeout );

    return { flag, buf };
}

QPair< bool, QByteArray > JQNet::HTTP::patch(const QNetworkRequest &request, const QByteArray &body, const int &timeout)
{
    QByteArray buf;
    HTTP http;

    const auto &&flag = http.patch( request, body, buf, timeout );

    return { flag, buf };
}
#endif

void JQNet::HTTP::handle(
        QNetworkReply *reply, const int &timeout,
        const std::function<void (const QByteArray &)> &onFinished,
        const std::function<void (const QNetworkReply::NetworkError &, const QByteArray &data)> &onError,
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

//        qDebug() << acceptedData;

        onFinished( acceptedData );
    } );

#ifndef QT_NO_SSL
    if ( reply->url().toString().toLower().startsWith( "https" ) )
    {
        QObject::connect( reply, static_cast< void( QNetworkReply::* )( const QList< QSslError > & ) >( &QNetworkReply::sslErrors ), [ reply ](const QList< QSslError > &errors)
        {
            qDebug() << "HTTP::handle: ignoreSslErrors:" << errors;
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

//        qDebug() << acceptedData;

        onError( code, acceptedData );
    } );
}
