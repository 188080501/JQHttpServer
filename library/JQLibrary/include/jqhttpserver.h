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

#ifndef JQLIBRARY_INCLUDE_JQHTTPSERVER_H_
#define JQLIBRARY_INCLUDE_JQHTTPSERVER_H_

#ifndef QT_NETWORK_LIB
#   error("Please add network in pro file")
#endif

#ifndef QT_CONCURRENT_LIB
#   error("Please add concurrent in pro file")
#endif

// C++ lib import
#include <functional>

// Qt lib import
#include <QObject>
#include <QSharedPointer>
#include <QPointer>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QMutex>
#include <QHostAddress>
#include <QUrl>
#include <QTcpSocket>
#include <QIODevice>
#ifndef QT_NO_SSL
#   include <QSslCertificate>
#   include <QSslSocket>
#endif

// JQLibrary lib import
#include <JQDeclare>

class QThreadPool;
class QTimer;
class QImage;
class QTcpServer;
class QLocalServer;
class QSslKey;
class QSslConfiguration;

namespace JQHttpServer
{

class JQLIBRARY_EXPORT Session: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY( Session )

public:
    Session( const QPointer< QTcpSocket > &socket );

    virtual ~Session() override;

    inline void setHandleAcceptedCallback(const std::function< void(const QPointer< Session > &) > &callback) { handleAcceptedCallback_ = callback; }

    inline QPointer< QTcpSocket > socket() { return socket_; }


    QString requestSourceIp() const;

    QString requestMethod() const;

    QString requestUrl() const;

    QString requestCrlf() const;

    QMap< QString, QString > requestHeader() const;

    QByteArray requestBody() const;

    QString requestUrlPath() const;

    QStringList requestUrlPathSplitToList() const;

    QMap< QString, QString > requestUrlQuery() const;

    int replyHttpCode() const;

    qint64 replyBodySize() const;

#ifndef QT_NO_SSL
    QSslCertificate peerCertificate() const;
#endif

public slots:
    void replyText(const QString &replyData, const int &httpStatusCode = 200);

    void replyRedirects(const QUrl &targetUrl, const int &httpStatusCode = 200);

    void replyJsonObject(const QJsonObject &jsonObject, const int &httpStatusCode = 200);

    void replyJsonArray(const QJsonArray &jsonArray, const int &httpStatusCode = 200);

    void replyFile(const QString &filePath, const int &httpStatusCode = 200);

    void replyFile(const QString &fileName, const QByteArray &fileData, const int &httpStatusCode = 200);

    void replyImage(const QImage &image, const QString &format = "PNG", const int &httpStatusCode = 200);

    void replyImage(const QString &imageFilePath, const int &httpStatusCode = 200);

    void replyBytes(const QByteArray &bytes, const QString &contentType = "application/octet-stream", const int &httpStatusCode = 200, const QString &exHeader = QString());

    void replyOptions();

private:
    void analyseBufferSetup1();

    void analyseBufferSetup2();

    void onBytesWritten(const qint64 &written);

    void onStateChanged(const QAbstractSocket::SocketState &socketState);

private:
    static QAtomicInt remainSession_;

    QPointer< QTcpSocket >                               socket_;
    std::function< void( const QPointer< Session > & ) > handleAcceptedCallback_;
    QSharedPointer< QTimer >                             autoCloseTimer_;

    QByteArray receiveBuffer_;

    QString                  requestSourceIp_;
    QString                  requestMethod_;
    QString                  requestUrl_;
    QString                  requestCrlf_;
    QByteArray               requestBody_;
    QMap< QString, QString > requestHeader_;

    bool   headerAcceptedFinished_  = false;
    bool   contentAcceptedFinished_ = false;
    qint64 contentLength_           = -1;

    int        replyHttpCode_ = -1;
    QByteArray replyBuffer_;
    qint64     replyBodySize_ = -1;

    qint64                      waitWrittenByteCount_ = -1;
    QSharedPointer< QIODevice > replyIoDevice_;
};

class JQLIBRARY_EXPORT AbstractManage: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY( AbstractManage )

public:
    AbstractManage(const int &handleMaxThreadCount);

    virtual ~AbstractManage() override;

    inline void setHttpAcceptedCallback(const std::function< void(const QPointer< Session > &session) > &httpAcceptedCallback) { httpAcceptedCallback_ = httpAcceptedCallback; }

    inline QSharedPointer< QThreadPool > handleThreadPool() { return handleThreadPool_; }

    inline QSharedPointer< QThreadPool > serverThreadPool() { return serverThreadPool_; }

    virtual bool isRunning() = 0;

protected Q_SLOTS:
    bool initialize();

    void deinitialize();

protected:
    virtual bool onStart() = 0;

    virtual void onFinish() = 0;

    bool startServerThread();

    void stopHandleThread();

    void stopServerThread();

    void newSession(const QPointer< Session > &session);

    void handleAccepted(const QPointer< Session > &session);

signals:
    void readyToClose();

protected:
    QSharedPointer< QThreadPool > serverThreadPool_;
    QSharedPointer< QThreadPool > handleThreadPool_;

    QMutex mutex_;

    std::function< void(const QPointer< Session > &session) > httpAcceptedCallback_;

    QSet< Session * > availableSessions_;
};

class JQLIBRARY_EXPORT TcpServerManage: public AbstractManage
{
    Q_OBJECT
    Q_DISABLE_COPY( TcpServerManage )

public:
    TcpServerManage(const int &handleMaxThreadCount = 2);

    virtual ~TcpServerManage() override;

    bool listen( const QHostAddress &address, const quint16 &port );

private:
    bool isRunning() override;

    bool onStart() override;

    void onFinish() override;

private:
    QPointer< QTcpServer > tcpServer_;

    QHostAddress listenAddress_ = QHostAddress::Any;
    quint16 listenPort_ = 0;
};

#ifndef QT_NO_SSL
class SslServerHelper;

class JQLIBRARY_EXPORT SslServerManage: public AbstractManage
{
    Q_OBJECT
    Q_DISABLE_COPY( SslServerManage )

public:
    SslServerManage(const int &handleMaxThreadCount = 2);

    virtual ~SslServerManage() override;

    bool listen( const QHostAddress &                                   address,
                 const quint16 &                                        port,
                 const QString &                                        crtFilePath,
                 const QString &                                        keyFilePath,
                 const QList< QPair< QString, QSsl::EncodingFormat > > &caFileList = {},    // [ { filePath, format } ]
                 const QSslSocket::PeerVerifyMode &                     peerVerifyMode = QSslSocket::VerifyNone );

private:
    bool isRunning() override;

    bool onStart() override;

    void onFinish() override;

private:
    QPointer< SslServerHelper > tcpServer_;

    QHostAddress listenAddress_ = QHostAddress::Any;
    quint16      listenPort_    = 0;

    QSharedPointer< QSslConfiguration > sslConfiguration_;
};


enum ServiceConfigEnum
{
    ServiceUnknownConfig,
    ServiceHttpListenPort,
    ServiceHttpsListenPort,
    ServiceProcessor, // QPointer< QObject > or QList< QPointer< QObject > >
    ServiceUuid,
    ServiceSslCrtFilePath,
    ServiceSslKeyFilePath,
    ServiceSslCAFilePath,
    ServiceSslPeerVerifyMode,
};

class Service: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY( Service )

private:
    enum ReceiveDataType
    {
        UnknownReceiveDataType,
        NoReceiveDataType,
        VariantListReceiveDataType,
        VariantMapReceiveDataType,
        ListVariantMapReceiveDataType,
    };

    struct ApiConfig
    {
        QPointer< QObject > processor;
        QString             apiMethod;
        QString             apiName;
        QString             slotName;
        ReceiveDataType     receiveDataType = UnknownReceiveDataType;
    };

    class Recoder
    {
    public:
        Recoder( const QPointer< JQHttpServer::Session > &session );

        ~Recoder();

        QPointer< JQHttpServer::Session > session_;
        QDateTime                         acceptedTime_;
        QString                           serviceUuid_;
        QString                           apiName;
    };

protected:
    Service() = default;

public:
    virtual ~Service() override = default;


    static QSharedPointer< Service > createService( const QMap< ServiceConfigEnum, QVariant > &config );


    void registerProcessor( const QPointer< QObject > &processor );


    virtual QJsonDocument extractPostJsonData( const QPointer< JQHttpServer::Session > &session );

    static void reply(
        const QPointer< JQHttpServer::Session > &session,
        const QJsonObject &data,
        const bool &isSucceed = true,
        const QString &message = { },
        const int &httpStatusCode = 200 );

    static void reply(
        const QPointer< JQHttpServer::Session > &session,
        const bool &isSucceed = true,
        const QString &message = { },
        const int &httpStatusCode = 200 );


    virtual void httpGetPing( const QPointer< JQHttpServer::Session > &session );

    virtual void httpGetFaviconIco( const QPointer< JQHttpServer::Session > &session );

    virtual void httpOptions( const QPointer< JQHttpServer::Session > &session );

protected:
    bool initialize( const QMap< ServiceConfigEnum, QVariant > &config );

private:
    void onSessionAccepted( const QPointer< JQHttpServer::Session > &session );


    static QString snakeCaseToCamelCase(const QString &source, const bool &firstCharUpper = false);

    static QList< QVariantMap > variantListToListVariantMap(const QVariantList &source);

private:
    QSharedPointer< JQHttpServer::TcpServerManage > httpServerManage_;
    QSharedPointer< JQHttpServer::SslServerManage > httpsServerManage_;

    QString                                     serviceUuid_;
    QMap< QString, QMap< QString, ApiConfig > > schedules_;    // apiMethod -> apiName -> API
    QMap< QString, std::function< void( const QPointer< JQHttpServer::Session > &session ) > > schedules2_; // apiPathPrefix -> callback
    QPointer< QObject > certificateVerifier_;
};
#endif

}

#endif//JQLIBRARY_INCLUDE_JQHTTPSERVER_H_
