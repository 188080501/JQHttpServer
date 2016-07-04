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

#ifndef __JQNet_h__
#define __JQNet_h__

#ifndef QT_NETWORK_LIB
#   error("Plwaer add network in pro file")
#endif

// C++ lib import
#include <functional>

// Qt lib import
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkInterface>
#include <QNetworkAddressEntry>

namespace JQNet
{

QNetworkAddressEntry getNetworkAddressEntry();

QPair< QNetworkAddressEntry, QNetworkInterface > getNetworkAddressEntryWithNetworkInterface(const bool &ridVm = true);

QString getHostName();

class HTTP
{
public:
    inline QNetworkAccessManager &manage() { return manage_; }


    bool get(const QNetworkRequest &request, QByteArray &target, const int &timeout = 30000);

    void get(const QNetworkRequest &request,
             const std::function< void(const QByteArray &data) > &onFinished,
             const std::function< void(const QNetworkReply::NetworkError &code) > &onError,
             const int &timeout = 30000);

    bool post(const QNetworkRequest &request, const QByteArray &appendData, QByteArray &target, const int &timeout);

    void post(const QNetworkRequest &request,
              const QByteArray &appendData,
              const std::function< void(const QByteArray &data) > &onFinished,
              const std::function< void(const QNetworkReply::NetworkError &code) > &onError,
              const int &timeout = 30000);


    static QPair< bool, QByteArray > get(const QString &url, const int &timeout = 30000);

    static QPair< bool, QByteArray > get(const QNetworkRequest &request, const int &timeout = 30000);

    static QPair< bool, QByteArray > post(const QString &url, const QByteArray &appendData, const int &timeout = 30000);

    static QPair< bool, QByteArray > post(const QNetworkRequest &request, const QByteArray &appendData, const int &timeout = 30000);

private:
    void handle(QNetworkReply *reply, const int &timeout,
                const std::function< void(const QByteArray &data) > &onFinished,
                const std::function< void(const QNetworkReply::NetworkError &code) > &onError,
                const std::function< void() > &onTimeout);

private:
    QNetworkAccessManager manage_;
};

}

#endif//__JQNet_h__
