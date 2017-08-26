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

#ifndef JQNET_H_
#define JQNET_H_

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

namespace JQNet
{

QNetworkAddressEntry getNetworkAddressEntry();

QPair< QNetworkAddressEntry, QNetworkInterface > getNetworkAddressEntryWithNetworkInterface(const bool &ridVm = true);

QString getHostName();

class HTTP
{
    Q_DISABLE_COPY( HTTP )

public:
    HTTP() = default;

    ~HTTP() = default;

public:
    inline QNetworkAccessManager &manage() { return manage_; }


    bool get(
            const QNetworkRequest &request,
            QByteArray &target,
            const int &timeout = 30 * 1000
        );

    void get(
            const QNetworkRequest &request,
            const std::function< void(const QByteArray &data) > &onFinished,
            const std::function< void(const QNetworkReply::NetworkError &code, const QByteArray &data) > &onError,
            const int &timeout = 30 * 1000
        );

    bool deleteResource(
            const QNetworkRequest &request,
            QByteArray &target,
            const int &timeout = 30 * 1000
        );

    void deleteResource(
            const QNetworkRequest &request,
            const std::function< void(const QByteArray &data) > &onFinished,
            const std::function< void(const QNetworkReply::NetworkError &code, const QByteArray &data) > &onError,
            const int &timeout = 30 * 1000
        );

    bool post(
            const QNetworkRequest &request,
            const QByteArray &body,
            QByteArray &target,
            const int &timeout = 30 * 1000
        );

    bool post(
            const QNetworkRequest &request,
            const QSharedPointer< QHttpMultiPart > &multiPart,
            QByteArray &target,
            const int &timeout = 30 * 1000
        );

    void post(
            const QNetworkRequest &request,
            const QByteArray &body,
            const std::function< void(const QByteArray &data) > &onFinished,
            const std::function< void(const QNetworkReply::NetworkError &code, const QByteArray &data) > &onError,
            const int &timeout = 30 * 1000
        );

    bool put(
            const QNetworkRequest &request,
            const QByteArray &body,
            QByteArray &target,
            const int &timeout = 30 * 1000
        );

    bool put(
            const QNetworkRequest &request,
            const QSharedPointer< QHttpMultiPart > &multiPart,
            QByteArray &target,
            const int &timeout = 30 * 1000
        );

    void put(
            const QNetworkRequest &request,
            const QByteArray &body,
            const std::function< void(const QByteArray &data) > &onFinished,
            const std::function< void(const QNetworkReply::NetworkError &code, const QByteArray &data) > &onError,
            const int &timeout = 30 * 1000
        );

#if !( defined Q_OS_LINUX ) && ( QT_VERSION >= QT_VERSION_CHECK( 5, 9, 0 ) )
    bool patch(
            const QNetworkRequest &request,
            const QByteArray &body,
            QByteArray &target,
            const int &timeout = 30 * 1000
        );

    void patch(
            const QNetworkRequest &request,
            const QByteArray &body,
            const std::function< void(const QByteArray &data) > &onFinished,
            const std::function< void(const QNetworkReply::NetworkError &code, const QByteArray &data) > &onError,
            const int &timeout = 30 * 1000
        );
#endif


    static QPair< bool, QByteArray > get(const QString &url, const int &timeout = 30 * 1000);

    static QPair< bool, QByteArray > get(const QNetworkRequest &request, const int &timeout = 30 * 1000);

    static QPair< bool, QByteArray > deleteResource(const QString &url, const int &timeout = 30 * 1000);

    static QPair< bool, QByteArray > deleteResource(const QNetworkRequest &request, const int &timeout = 30 * 1000);

    static QPair< bool, QByteArray > post(const QString &url, const QByteArray &body, const int &timeout = 30 * 1000);

    static QPair< bool, QByteArray > post(const QNetworkRequest &request, const QByteArray &body, const int &timeout = 30 * 1000);

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
            QNetworkReply *reply, const int &timeout,
            const std::function< void(const QByteArray &data) > &onFinished,
            const std::function< void(const QNetworkReply::NetworkError &code, const QByteArray &data) > &onError,
            const std::function< void() > &onTimeout
        );

private:
    QNetworkAccessManager manage_;
};

}

#endif//JQNET_H_
