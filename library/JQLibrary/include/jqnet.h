/*
    This file is part of JQLibrary

    Copyright: Jason and others

    Contact email: 188080501@qq.com

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
    LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef JQLIBRARY_INCLUDE_JQNET_H_
#define JQLIBRARY_INCLUDE_JQNET_H_

#ifndef QT_NETWORK_LIB
#   error("Please add network in pro file")
#endif

// C++ lib import
#include <functional>

// Qt lib import
#include <QSharedPointer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QNetworkInterface>
#include <QNetworkAddressEntry>

// JQLibrary lib import
#include <JQDeclare>

namespace JQNet
{

QNetworkAddressEntry getFirstNetworkAddressEntry();

QPair< QNetworkAddressEntry, QNetworkInterface > getFirstNetworkAddressEntryAndInterface(const bool &ridVm = true);

QList< QPair< QNetworkAddressEntry, QNetworkInterface > > getNetworkAddressEntryAndInterface(const bool &ridVm = true);

QString getHostName();

bool tcpReachable(const QString &hostName, const quint16 &port, const int &timeout = 5000);

#ifdef JQFOUNDATION_LIB
bool pingReachable(const QString &address, const int &timeout = 300);
#endif

class JQLIBRARY_EXPORT HTTP
{
    Q_DISABLE_COPY( HTTP )

public:
    HTTP() = default;

    ~HTTP() = default;

public:
    inline QNetworkAccessManager &manage() { return manage_; }


    bool get(
            const QNetworkRequest &request,
            QByteArray &receiveBuffer,
            const int &timeout = 30 * 1000
        );

    void get(
            const QNetworkRequest &request,
            const std::function< void(const QList< QNetworkReply::RawHeaderPair > &, const QByteArray &data) > &onFinished,
            const std::function< void(const QList< QNetworkReply::RawHeaderPair > &, const QNetworkReply::NetworkError &code, const QByteArray &data) > &onError,
            const int &timeout = 30 * 1000
        );

    bool deleteResource(
            const QNetworkRequest &request,
            QByteArray &receiveBuffer,
            const int &timeout = 30 * 1000
        );

    void deleteResource(
            const QNetworkRequest &request,
            const std::function< void(const QList< QNetworkReply::RawHeaderPair > &, const QByteArray &data) > &onFinished,
            const std::function< void(const QList< QNetworkReply::RawHeaderPair > &, const QNetworkReply::NetworkError &code, const QByteArray &data) > &onError,
            const int &timeout = 30 * 1000
        );

    bool post(
            const QNetworkRequest &request,
            const QByteArray &body,
            QList< QNetworkReply::RawHeaderPair > &receiveRawHeaderPairs,
            QByteArray &receiveBuffer,
            const int &timeout = 30 * 1000
        );

    bool post(
            const QNetworkRequest &request,
            const QSharedPointer< QHttpMultiPart > &multiPart,
            QByteArray &receiveBuffer,
            const int &timeout = 30 * 1000
        );

    void post(
            const QNetworkRequest &request,
            const QByteArray &body,
            const std::function< void(const QList< QNetworkReply::RawHeaderPair > &, const QByteArray &data) > &onFinished,
            const std::function< void(const QList< QNetworkReply::RawHeaderPair > &, const QNetworkReply::NetworkError &code, const QByteArray &data) > &onError,
            const int &timeout = 30 * 1000
        );

    bool put(
            const QNetworkRequest &request,
            const QByteArray &body,
            QByteArray &receiveBuffer,
            const int &timeout = 30 * 1000
        );

    bool put(
            const QNetworkRequest &request,
            const QSharedPointer< QHttpMultiPart > &multiPart,
            QByteArray &receiveBuffer,
            const int &timeout = 30 * 1000
        );

    void put(
            const QNetworkRequest &request,
            const QByteArray &body,
            const std::function< void(const QList< QNetworkReply::RawHeaderPair > &, const QByteArray &data) > &onFinished,
            const std::function< void(const QList< QNetworkReply::RawHeaderPair > &, const QNetworkReply::NetworkError &code, const QByteArray &data) > &onError,
            const int &timeout = 30 * 1000
        );

#if !( defined Q_OS_LINUX ) && ( QT_VERSION >= QT_VERSION_CHECK( 5, 9, 0 ) )
    bool patch(
            const QNetworkRequest &request,
            const QByteArray &body,
            QByteArray &receiveBuffer,
            const int &timeout = 30 * 1000
        );

    void patch(
            const QNetworkRequest &request,
            const QByteArray &body,
            const std::function< void(const QList< QNetworkReply::RawHeaderPair > &, const QByteArray &data) > &onFinished,
            const std::function< void(const QList< QNetworkReply::RawHeaderPair > &, const QNetworkReply::NetworkError &code, const QByteArray &data) > &onError,
            const int &timeout = 30 * 1000
        );
#endif


    static QPair< bool, QByteArray > get(const QString &url, const int &timeout = 30 * 1000);

    static QPair< bool, QByteArray > get(const QNetworkRequest &request, const int &timeout = 30 * 1000);

    static QPair< bool, QByteArray > deleteResource(const QString &url, const int &timeout = 30 * 1000);

    static QPair< bool, QByteArray > deleteResource(const QNetworkRequest &request, const int &timeout = 30 * 1000);

    static QPair< bool, QByteArray > post(const QString &url, const QByteArray &body, const int &timeout = 30 * 1000);

    static QPair< bool, QByteArray > post(const QNetworkRequest &request, const QByteArray &body, const int &timeout = 30 * 1000);

    static QPair< bool, QPair< QList< QNetworkReply::RawHeaderPair >, QByteArray > > post2(const QNetworkRequest &request, const QByteArray &body, const int &timeout = 30 * 1000);

    static QPair< bool, QByteArray > post(const QNetworkRequest &request, const QSharedPointer< QHttpMultiPart > &multiPart, const int &timeout = 30 * 1000);

    static QPair< bool, QByteArray > put(const QString &url, const QByteArray &body, const int &timeout = 30 * 1000);

    static QPair< bool, QByteArray > put(const QNetworkRequest &request, const QByteArray &body, const int &timeout = 30 * 1000);

    static QPair< bool, QByteArray > put(const QNetworkRequest &request, const QSharedPointer< QHttpMultiPart > &multiPart, const int &timeout = 30 * 1000);

#if !( defined Q_OS_LINUX ) && ( QT_VERSION >= QT_VERSION_CHECK( 5, 9, 0 ) )
    static QPair< bool, QByteArray > patch(const QString &url, const QByteArray &body, const int &timeout = 30 * 1000);

    static QPair< bool, QByteArray > patch(const QNetworkRequest &request, const QByteArray &body, const int &timeout = 30 * 1000);
#endif

private:
    void handle(
            QNetworkReply *reply,
            const int &timeout,
            const std::function< void(const QList< QNetworkReply::RawHeaderPair > &, const QByteArray &data) > &onFinished,
            const std::function< void(const QList< QNetworkReply::RawHeaderPair > &, const QNetworkReply::NetworkError &code, const QByteArray &data) > &onError,
            const std::function< void() > &onTimeout
        );

private:
    QNetworkAccessManager manage_;
};

}

#endif//JQLIBRARY_INCLUDE_JQNET_H_
