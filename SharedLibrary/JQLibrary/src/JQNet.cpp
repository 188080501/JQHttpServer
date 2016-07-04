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

#include "JQNet.h"

// Qt lib import
#include <QEventLoop>
#include <QTimer>
#include <QUrl>
#include <QHostInfo>

using namespace JQNet;

QNetworkAddressEntry JQNet::getNetworkAddressEntry()
{
    return getNetworkAddressEntryWithNetworkInterface().first;
}

QPair< QNetworkAddressEntry, QNetworkInterface > JQNet::getNetworkAddressEntryWithNetworkInterface(const bool &ridVm)
{
    for (const auto &interface: QNetworkInterface::allInterfaces())
    {
        if ( interface.flags() != (QNetworkInterface::IsUp
                                 | QNetworkInterface::IsRunning
                                 | QNetworkInterface::CanBroadcast
                                 | QNetworkInterface::CanMulticast ) ) { continue; }

        if ( ridVm && interface.humanReadableName().startsWith( "vm" ) ) { continue; }

        for( const auto &entry: interface.addressEntries() )
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
#ifdef Q_OS_MAC
    return QHostInfo::localHostName().replace(".local", "");
#endif
    return QHostInfo::localHostName();
}

// HTTP
bool HTTP::get(const QNetworkRequest &request, QByteArray &target, const int &timeout)
{
    target.clear();

    QEventLoop eventLoop;
    auto reply = manage_.get(request);
    bool failFlag = false;

    this->handle(
        reply,
        timeout,
        [&](const QByteArray &data)
        {
            target = data;
            eventLoop.exit(1);
        },
        [&](const QNetworkReply::NetworkError &)
        {
            eventLoop.exit(0);
        },
        [&]()
        {
            failFlag = true;
            eventLoop.exit(0);
        }
    );

    return eventLoop.exec() && !failFlag;
}

void HTTP::get(const QNetworkRequest &request,
               const std::function<void (const QByteArray &)> &onFinished,
               const std::function<void (const QNetworkReply::NetworkError &)> &onError,
               const int &timeout)
{
    auto reply = manage_.get(request);

    this->handle(
        reply,
        timeout,
        onFinished,
        onError,
        [=]()
        {
            onError(QNetworkReply::TimeoutError);
        }
    );
}

bool HTTP::post(const QNetworkRequest &request, const QByteArray &appendData, QByteArray &target, const int &timeout)
{
    target.clear();

    QEventLoop eventLoop;
    auto reply = manage_.post(request, appendData);
    bool failFlag = false;

    this->handle(
        reply,
        timeout,
        [&](const QByteArray &data)
        {
            target = data;
            eventLoop.exit(1);
        },
        [&](const QNetworkReply::NetworkError &)
        {
            eventLoop.exit(0);
        },
        [&]()
        {
            failFlag = true;
            eventLoop.exit(0);
        }
    );

    return eventLoop.exec() && !failFlag;
}

void HTTP::post(const QNetworkRequest &request,
                const QByteArray &appendData,
                const std::function<void (const QByteArray &)> &onFinished,
                const std::function<void (const QNetworkReply::NetworkError &)> &onError,
                const int &timeout)
{
    auto reply = manage_.post(request, appendData);

    this->handle(
        reply,
        timeout,
        onFinished,
        onError,
        [=]()
        {
            onError( QNetworkReply::TimeoutError );
        }
    );
}

QPair< bool, QByteArray > HTTP::get(const QString &url, const int &timeout)
{
    QNetworkRequest networkRequest( ( QUrl( url ) ) );
    QByteArray buf;

    const auto &&flag = HTTP().get( networkRequest, buf, timeout );

    return { flag, buf };
}

QPair< bool, QByteArray > HTTP::get(const QNetworkRequest &request, const int &timeout)
{
    QByteArray buf;
    HTTP http;

    const auto &&flag = http.get( request, buf, timeout );

    return { flag, buf };
}

QPair< bool, QByteArray > HTTP::post(const QString &url, const QByteArray &appendData, const int &timeout)
{
    QNetworkRequest networkRequest( ( QUrl( url ) ) );
    QByteArray buf;

    networkRequest.setRawHeader( "Content-Type", "application/x-www-form-urlencoded" );

    const auto &&flag = HTTP().post( networkRequest, appendData, buf, timeout );

    return { flag, buf };
}

QPair< bool, QByteArray > HTTP::post(const QNetworkRequest &request, const QByteArray &appendData, const int &timeout)
{
    QByteArray buf;
    HTTP http;

    const auto &&flag = http.post( request, appendData, buf, timeout );

    return { flag, buf };
}

void HTTP::handle(QNetworkReply *reply, const int &timeout,
                  const std::function<void (const QByteArray &)> &onFinished,
                  const std::function<void (const QNetworkReply::NetworkError &)> &onError,
                  const std::function<void ()> &onTimeout)
{
    QTimer *timer = NULL;
    if (timeout)
    {
        timer = new QTimer;
        timer->setSingleShot(true);
        QObject::connect(timer, &QTimer::timeout, [=]()
        {
            onTimeout();
            timer->deleteLater();
        });
        timer->start(timeout);
    }

    QObject::connect(reply, &QNetworkReply::finished, [=]()
    {
        if (timer)
        {
            timer->deleteLater();
        }
        onFinished(reply->readAll());
    });

    QObject::connect(reply, (void(QNetworkReply::*)(QNetworkReply::NetworkError))&QNetworkReply::error, [=](const QNetworkReply::NetworkError &code)
    {
        if (timer)
        {
            timer->deleteLater();
        }
        onError(code);
    });
}
