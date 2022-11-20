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

#include <JQHttpServer>

// Qt lib import
#include <QEventLoop>
#include <QTimer>
#include <QSemaphore>
#include <QMetaObject>
#include <QThread>
#include <QThreadPool>
#include <QJsonDocument>
#include <QJsonValue>
#include <QPointer>
#include <QFile>
#include <QImage>
#include <QBuffer>
#include <QPainter>
#include <QtConcurrent>

#include <QTcpServer>
#include <QTcpSocket>
#include <QLocalServer>
#include <QLocalSocket>
#ifndef QT_NO_SSL
#   include <QSslKey>
#   include <QSslConfiguration>
#endif

#define JQHTTPSERVER_SESSION_PROTECTION( functionName, ... )                             \
    auto this_ = this;                                                                   \
    if ( !this_ || ( contentLength_ < -1 ) || ( waitWrittenByteCount_ < -1 ) )           \
    {                                                                                    \
        qDebug().noquote() << QStringLiteral( "JQHttpServer::Session::" ) + functionName \
                                  + ": current session this is null";                    \
        return __VA_ARGS__;                                                              \
    }

#define JQHTTPSERVER_SESSION_REPLY_PROTECTION( functionName, ... )                                            \
    JQHTTPSERVER_SESSION_PROTECTION( functionName, __VA_ARGS__ )                                              \
    if ( ( replyHttpCode_ >= 0 ) && ( QThread::currentThread() != this->thread() ) )                          \
    {                                                                                                         \
        qDebug().noquote() << QStringLiteral( "JQHttpServer::Session::" ) + functionName + ": already reply"; \
        return __VA_ARGS__;                                                                                   \
    }

#define JQHTTPSERVER_SESSION_REPLY_PROTECTION2( functionName, ... )                                    \
    if ( socket_.isNull() )                                                                            \
    {                                                                                                  \
        qDebug().noquote() << QStringLiteral( "JQHttpServer::Session::" ) + functionName + ": error1"; \
        this->deleteLater();                                                                           \
        return __VA_ARGS__;                                                                            \
    }

static QString replyTextFormat(
        "HTTP/1.1 %1 OK\r\n"
        "Content-Type: %2\r\n"
        "Content-Length: %3\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Headers: Content-Type,X-Requested-With\r\n"
        "\r\n"
        "%4"
    );

static QString replyRedirectsFormat(
        "HTTP/1.1 %1 OK\r\n"
        "Content-Type: %2\r\n"
        "Content-Length: %3\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Headers: Content-Type,X-Requested-With\r\n"
        "\r\n"
        "%4"
    );

static QString replyFileFormat(
        "HTTP/1.1 %1 OK\r\n"
        "Content-Disposition: attachment;filename=%2\r\n"
        "Content-Length: %3\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Headers: Content-Type,X-Requested-With\r\n"
        "\r\n"
    );

static QString replyImageFormat(
        "HTTP/1.1 %1\r\n"
        "Content-Type: image/%2\r\n"
        "Content-Length: %3\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Headers: Content-Type,X-Requested-With\r\n"
        "\r\n"
    );

static QString replyBytesFormat(
        "HTTP/1.1 %1 OK\r\n"
        "Content-Type: %2\r\n"
        "Content-Length: %3\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Headers: Content-Type,X-Requested-With\r\n"
        "%4"
        "\r\n"
    );

static QString replyOptionsFormat(
        "HTTP/1.1 200 OK\r\n"
        "Allow: OPTIONS, GET, POST, PUT, HEAD\r\n"
        "Access-Control-Allow-Methods: OPTIONS, GET, POST, PUT, HEAD\r\n"
        "Content-Length: 0\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Headers: Content-Type,X-Requested-With\r\n"
        "\r\n"
    );

// Session
QAtomicInt JQHttpServer::Session::remainSession_ = 0;

JQHttpServer::Session::Session(const QPointer< QTcpSocket > &socket):
    socket_( socket ),
    autoCloseTimer_( new QTimer )
{
    ++remainSession_;
//    qDebug() << "remainSession:" << remainSession_ << this;

    if ( qobject_cast< QAbstractSocket * >( socket ) )
    {
        requestSourceIp_ = ( qobject_cast< QAbstractSocket * >( socket ) )->peerAddress().toString().replace( "::ffff:", "" );
    }

    connect( socket_.data(), &QTcpSocket::readyRead, [ this ]()
    {
        autoCloseTimer_->stop();

        this->receiveBuffer_.append( this->socket_->readAll() );
        this->analyseBufferSetup1();

        autoCloseTimer_->start();
    } );

#ifndef QT_NO_SSL
    if ( qobject_cast< QSslSocket * >( socket ) )
    {
        connect( qobject_cast< QSslSocket * >( socket ),
                 &QSslSocket::encryptedBytesWritten,
                 std::bind( &JQHttpServer::Session::onBytesWritten, this, std::placeholders::_1 ) );
    }
    else
#endif
    {
        connect( socket_.data(),
                 &QTcpSocket::bytesWritten,
                 std::bind( &JQHttpServer::Session::onBytesWritten, this, std::placeholders::_1 ) );
    }

    if ( qobject_cast< QTcpSocket * >( socket ) )
    {
        connect( qobject_cast< QTcpSocket * >( socket ),
                &QAbstractSocket::stateChanged,
                std::bind( &JQHttpServer::Session::onStateChanged, this, std::placeholders::_1 ) );
    }

    autoCloseTimer_->setInterval( 30 * 1000 );
    autoCloseTimer_->setSingleShot( true );
    autoCloseTimer_->start();

    connect( autoCloseTimer_.data(), &QTimer::timeout, this, &QObject::deleteLater );
}

JQHttpServer::Session::~Session()
{
    --remainSession_;
//    qDebug() << "remainSession:" << remainSession_ << this;

    if ( !socket_.isNull() )
    {
        delete socket_.data();
    }
}

QString JQHttpServer::Session::requestSourceIp() const
{
    JQHTTPSERVER_SESSION_PROTECTION( "requestSource", { } )

    return requestSourceIp_;
}

QString JQHttpServer::Session::requestMethod() const
{
    JQHTTPSERVER_SESSION_PROTECTION( "requestMethod", { } )

    return requestMethod_;
}

QString JQHttpServer::Session::requestUrl() const
{
    JQHTTPSERVER_SESSION_PROTECTION( "requestUrl", { } )

    return requestUrl_;
}

QString JQHttpServer::Session::requestCrlf() const
{
    JQHTTPSERVER_SESSION_PROTECTION( "requestCrlf", { } )

    return requestCrlf_;
}

QMap< QString, QString > JQHttpServer::Session::requestHeader() const
{
    JQHTTPSERVER_SESSION_PROTECTION( "requestHeader", { } )

    return requestHeader_;
}

QByteArray JQHttpServer::Session::requestBody() const
{
    JQHTTPSERVER_SESSION_PROTECTION( "requestBody", { } )

    return requestBody_;
}

QString JQHttpServer::Session::requestUrlPath() const
{
    JQHTTPSERVER_SESSION_PROTECTION( "requestUrlPath", { } )

    QString result;
    const auto indexForQueryStart = requestUrl_.indexOf( "?" );

    if ( indexForQueryStart >= 0 )
    {
        result = requestUrl_.mid( 0, indexForQueryStart );
    }
    else
    {
        result = requestUrl_;
    }

    if ( result.startsWith( "//" ) )
    {
        result = result.mid( 1 );
    }

    if ( result.endsWith( "/" ) )
    {
        result = result.mid( 0, result.size() - 1 );
    }

    result.replace( "%5B", "[" );
    result.replace( "%5D", "]" );
    result.replace( "%7B", "{" );
    result.replace( "%7D", "}" );
    result.replace( "%5E", "^" );

    return result;
}

QStringList JQHttpServer::Session::requestUrlPathSplitToList() const
{
    auto list = this->requestUrlPath().split( "/" );

    while ( !list.isEmpty() && list.first().isEmpty() )
    {
        list.pop_front();
    }

    while ( !list.isEmpty() && list.last().isEmpty() )
    {
        list.pop_back();
    }

    return list;
}

QMap< QString, QString > JQHttpServer::Session::requestUrlQuery() const
{
    const auto indexForQueryStart = requestUrl_.indexOf( "?" );
    if ( indexForQueryStart < 0 ) { return { }; }

    QMap< QString, QString > result;

    auto lines = QUrl::fromEncoded( requestUrl_.mid( indexForQueryStart + 1 ).toUtf8() ).toString().split( "&" );

    for ( const auto &line_: qAsConst( lines ) )
    {
        auto line = line_;
        line.replace( "%5B", "[" );
        line.replace( "%5D", "]" );
        line.replace( "%7B", "{" );
        line.replace( "%7D", "}" );
        line.replace( "%5E", "^" );

        auto indexOf = line.indexOf( "=" );
        if ( indexOf > 0 )
        {
            result[ line.mid( 0, indexOf ) ] = line.mid( indexOf + 1 );
        }
    }

    return result;
}

int JQHttpServer::Session::replyHttpCode() const
{
    JQHTTPSERVER_SESSION_PROTECTION( "replyHttpCode", -1 )

    return replyHttpCode_;
}

qint64 JQHttpServer::Session::replyBodySize() const
{
    JQHTTPSERVER_SESSION_PROTECTION( "replyBodySize", -1 )

    return replyBodySize_;
}

#ifndef QT_NO_SSL
QSslCertificate JQHttpServer::Session::peerCertificate() const
{
    JQHTTPSERVER_SESSION_PROTECTION( "peerCertificate", QSslCertificate() )

    if ( !qobject_cast< QSslSocket * >( socket_ ) ) { return QSslCertificate(); }

    return qobject_cast< QSslSocket * >( socket_ )->peerCertificate();
}
#endif

void JQHttpServer::Session::replyText(const QString &replyData, const int &httpStatusCode)
{
    JQHTTPSERVER_SESSION_REPLY_PROTECTION( "replyText" )

    if ( QThread::currentThread() != this->thread() )
    {
        replyHttpCode_ = httpStatusCode;
        replyBodySize_ = replyData.toUtf8().size();

        QMetaObject::invokeMethod(
            this,
            "replyText",
            Qt::QueuedConnection,
            Q_ARG( QString, replyData ),
            Q_ARG( int, httpStatusCode ) );
        return;
    }

    JQHTTPSERVER_SESSION_REPLY_PROTECTION2( "replyText" )

    const auto &&data = replyTextFormat
                            .arg(
                                QString::number( httpStatusCode ),
                                "text;charset=UTF-8",
                                QString::number( replyBodySize_ ),
                                replyData )
                            .toUtf8();

    waitWrittenByteCount_ = data.size();
    socket_->write( data );
}

void JQHttpServer::Session::replyRedirects(const QUrl &targetUrl, const int &httpStatusCode)
{
    JQHTTPSERVER_SESSION_REPLY_PROTECTION( "replyRedirects" )

    if ( QThread::currentThread() != this->thread() )
    {
        replyHttpCode_ = httpStatusCode;

        QMetaObject::invokeMethod( this, "replyRedirects", Qt::QueuedConnection, Q_ARG( QUrl, targetUrl ), Q_ARG( int, httpStatusCode ) );
        return;
    }

    JQHTTPSERVER_SESSION_REPLY_PROTECTION2( "replyRedirects" )

    const auto &&buffer = QString( "<head>\n<meta http-equiv=\"refresh\" content=\"0;URL=%1/\" />\n</head>" ).arg( targetUrl.toString() );
    replyBodySize_ = buffer.toUtf8().size();

    const auto &&data = replyRedirectsFormat
                            .arg(
                                QString::number( httpStatusCode ),
                                "text;charset=UTF-8",
                                QString::number( replyBodySize_ ),
                                buffer )
                            .toUtf8();

    waitWrittenByteCount_ = data.size();
    socket_->write( data );
}

void JQHttpServer::Session::replyJsonObject(const QJsonObject &jsonObject, const int &httpStatusCode)
{
    JQHTTPSERVER_SESSION_REPLY_PROTECTION( "replyJsonObject" )

    if ( QThread::currentThread() != this->thread() )
    {
        replyHttpCode_ = httpStatusCode;
        replyBuffer_ = QJsonDocument( jsonObject ).toJson( QJsonDocument::Compact );
        replyBodySize_ = replyBuffer_.size();

        QMetaObject::invokeMethod( this, "replyJsonObject", Qt::QueuedConnection, Q_ARG( QJsonObject, jsonObject ), Q_ARG( int, httpStatusCode ) );
        return;
    }

    JQHTTPSERVER_SESSION_REPLY_PROTECTION2( "replyJsonObject" )

    const auto &&buffer = replyTextFormat
                              .arg(
                                  QString::number( httpStatusCode ),
                                  "application/json;charset=UTF-8",
                                  QString::number( replyBodySize_ ),
                                  QString( replyBuffer_ ) )
                              .toUtf8();

    waitWrittenByteCount_ = buffer.size();
    socket_->write( buffer );
}

void JQHttpServer::Session::replyJsonArray(const QJsonArray &jsonArray, const int &httpStatusCode)
{
    JQHTTPSERVER_SESSION_REPLY_PROTECTION( "replyJsonArray" )

    if ( QThread::currentThread() != this->thread() )
    {
        replyHttpCode_ = httpStatusCode;
        replyBuffer_ = QJsonDocument( jsonArray ).toJson( QJsonDocument::Compact );
        replyBodySize_ = replyBuffer_.size();

        QMetaObject::invokeMethod( this, "replyJsonArray", Qt::QueuedConnection, Q_ARG( QJsonArray, jsonArray ), Q_ARG( int, httpStatusCode ) );
        return;
    }

    JQHTTPSERVER_SESSION_REPLY_PROTECTION2( "replyJsonArray" )

    const auto &&buffer = replyTextFormat
                              .arg(
                                  QString::number( httpStatusCode ),
                                  "application/json;charset=UTF-8",
                                  QString::number( replyBodySize_ ),
                                  QString( replyBuffer_ ) )
                              .toUtf8();

    waitWrittenByteCount_ = buffer.size();
    socket_->write( buffer );
}

void JQHttpServer::Session::replyFile(const QString &filePath, const int &httpStatusCode)
{
    JQHTTPSERVER_SESSION_REPLY_PROTECTION( "replyFile" )

    if ( QThread::currentThread() != this->thread() )
    {
        replyHttpCode_ = httpStatusCode;

        QMetaObject::invokeMethod( this, "replyFile", Qt::QueuedConnection, Q_ARG( QString, filePath ), Q_ARG( int, httpStatusCode ) );
        return;
    }

    JQHTTPSERVER_SESSION_REPLY_PROTECTION2( "replyFile" )

    replyIoDevice_.reset( new QFile( filePath ) );
    QPointer< QFile > file = ( qobject_cast< QFile * >( replyIoDevice_.data() ) );

    if ( !file->open( QIODevice::ReadOnly ) )
    {
        qDebug() << "JQHttpServer::Session::replyFile: open file error:" << filePath;
        replyIoDevice_.clear();
        this->deleteLater();
        return;
    }

    replyBodySize_ = file->size();

    const auto &&data = replyFileFormat
                            .arg(
                                QString::number( httpStatusCode ),
                                QFileInfo( filePath ).fileName(),
                                QString::number( replyBodySize_ ) )
                            .toUtf8();

    waitWrittenByteCount_ = data.size() + file->size();
    socket_->write( data );
}

void JQHttpServer::Session::replyFile(const QString &fileName, const QByteArray &fileData, const int &httpStatusCode)
{
    JQHTTPSERVER_SESSION_REPLY_PROTECTION( "replyFile" )

    if ( QThread::currentThread() != this->thread() )
    {
        replyHttpCode_ = httpStatusCode;

        QMetaObject::invokeMethod( this, "replyFile", Qt::QueuedConnection, Q_ARG( QString, fileName ), Q_ARG( QByteArray, fileData ), Q_ARG( int, httpStatusCode ) );
        return;
    }

    JQHTTPSERVER_SESSION_REPLY_PROTECTION2( "replyFile" )

    auto buffer = new QBuffer;
    buffer->setData( fileData );

    if ( !buffer->open( QIODevice::ReadWrite ) )
    {
        qDebug() << "JQHttpServer::Session::replyFile: open buffer error";
        delete buffer;
        this->deleteLater();
        return;
    }

    replyIoDevice_.reset( buffer );
    replyIoDevice_->seek( 0 );

    replyBodySize_ = fileData.size();

    const auto &&data =
        replyFileFormat
            .arg( QString::number( httpStatusCode ), fileName, QString::number( replyBodySize_ ) )
            .toUtf8();

    waitWrittenByteCount_ = data.size() + fileData.size();
    socket_->write( data );
}

void JQHttpServer::Session::replyImage(const QImage &image, const QString &format, const int &httpStatusCode)
{
    JQHTTPSERVER_SESSION_REPLY_PROTECTION( "replyImage" )

    if ( QThread::currentThread() != this->thread() )
    {
        replyHttpCode_ = httpStatusCode;

        QMetaObject::invokeMethod( this, "replyImage", Qt::QueuedConnection, Q_ARG( QImage, image ), Q_ARG( QString, format ), Q_ARG( int, httpStatusCode ) );
        return;
    }

    JQHTTPSERVER_SESSION_REPLY_PROTECTION2( "replyImage" )

    auto buffer = new QBuffer;

    if ( !buffer->open( QIODevice::ReadWrite ) )
    {
        qDebug() << "JQHttpServer::Session::replyImage: open buffer error";
        delete buffer;
        this->deleteLater();
        return;
    }

    if ( !image.save( buffer, format.toLatin1().constData() ) )
    {
        qDebug() << "JQHttpServer::Session::replyImage: save image to buffer error";
        delete buffer;
        this->deleteLater();
        return;
    }

    replyIoDevice_.reset( buffer );
    replyIoDevice_->seek( 0 );

    replyBodySize_ = buffer->buffer().size();

    const auto &&data =
        replyImageFormat
            .arg( QString::number( httpStatusCode ), format.toLower(), QString::number( replyBodySize_ ) )
            .toUtf8();

    waitWrittenByteCount_ = data.size() + buffer->buffer().size();
    socket_->write( data );
}

void JQHttpServer::Session::replyImage(const QString &imageFilePath, const int &httpStatusCode)
{
    JQHTTPSERVER_SESSION_REPLY_PROTECTION( "replyImage" )

    if ( QThread::currentThread() != this->thread() )
    {
        replyHttpCode_ = httpStatusCode;

        QMetaObject::invokeMethod( this, "replyImage", Qt::QueuedConnection, Q_ARG( QString, imageFilePath ), Q_ARG( int, httpStatusCode ) );
        return;
    }

    JQHTTPSERVER_SESSION_REPLY_PROTECTION2( "replyImage" )

    auto file = new QFile( imageFilePath );

    if ( !file->open( QIODevice::ReadOnly ) )
    {
        qDebug() << "JQHttpServer::Session::replyImage: open buffer error";
        delete file;
        this->deleteLater();
        return;
    }

    replyIoDevice_.reset( file );
    replyIoDevice_->seek( 0 );

    replyBodySize_ = file->size();

    const auto &&data =
        replyImageFormat
            .arg( QString::number( httpStatusCode ), QFileInfo( imageFilePath ).suffix(), QString::number( replyBodySize_ ) )
            .toUtf8();

    waitWrittenByteCount_ = data.size() + file->size();
    socket_->write( data );
}

void JQHttpServer::Session::replyBytes(const QByteArray &bytes, const QString &contentType, const int &httpStatusCode, const QString &exHeader)
{
    JQHTTPSERVER_SESSION_REPLY_PROTECTION( "replyBytes" )

    if ( QThread::currentThread() != this->thread( ))
    {
        replyHttpCode_ = httpStatusCode;

        QMetaObject::invokeMethod(this, "replyBytes", Qt::QueuedConnection, Q_ARG(QByteArray, bytes), Q_ARG(QString, contentType), Q_ARG(int, httpStatusCode), Q_ARG(QString, exHeader));
        return;
    }

    JQHTTPSERVER_SESSION_REPLY_PROTECTION2( "replyBytes" )

    auto buffer = new QBuffer;
    buffer->setData( bytes );

    if ( !buffer->open( QIODevice::ReadWrite ) )
    {
        qDebug() << "JQHttpServer::Session::replyBytes: open buffer error";
        delete buffer;
        this->deleteLater();
        return;
    }

    replyIoDevice_.reset( buffer );
    replyIoDevice_->seek( 0 );

    replyBodySize_ = buffer->buffer().size();

    const auto &&data =
        replyBytesFormat
            .arg( QString::number( httpStatusCode ), contentType, QString::number( replyBodySize_ ), exHeader )
            .toUtf8();

    waitWrittenByteCount_ = data.size() + buffer->buffer().size();
    socket_->write(data);
}

void JQHttpServer::Session::replyOptions()
{
    JQHTTPSERVER_SESSION_REPLY_PROTECTION( "replyOptions" )

    if ( QThread::currentThread() != this->thread() )
    {
        replyHttpCode_ = 200;

        QMetaObject::invokeMethod( this, "replyOptions", Qt::QueuedConnection );
        return;
    }

    JQHTTPSERVER_SESSION_REPLY_PROTECTION2( "replyOptions" )

    replyBodySize_ = 0;

    const auto &&buffer = replyOptionsFormat.toUtf8();

    waitWrittenByteCount_ = buffer.size();
    socket_->write( buffer );
}

void JQHttpServer::Session::analyseBufferSetup1()
{
    if ( !headerAcceptedFinished_ )
    {
        forever
        {
            static QByteArray splitFlag( "\r\n" );

            auto splitFlagIndex = receiveBuffer_.indexOf( splitFlag );

            // 没有获取到分割标记，意味着数据不全
            if ( splitFlagIndex == -1 )
            {
                // 没有获取到 method 但是缓冲区内已经有了数据，这可能是一个无效的连接
                if ( requestMethod_.isEmpty() && ( receiveBuffer_.size() > 4 ) )
                {
//                    qDebug() << "JQHttpServer::Session::inspectionBuffer: error0";
                    this->deleteLater();
                    return;
                }

                return;
            }

            // 如果未获取到 method 并且已经定位到了分割标记符，那么直接放弃这个连接
            if ( requestMethod_.isEmpty() && ( splitFlagIndex == 0 ) )
            {
//                qDebug() << "JQHttpServer::Session::inspectionBuffer: error1";
                this->deleteLater();
                return;
            }

            // 如果没有获取到 method 则先尝试分析 method
            if ( requestMethod_.isEmpty() )
            {
                auto requestLineDatas = receiveBuffer_.mid( 0, splitFlagIndex ).split( ' ' );
                receiveBuffer_.remove( 0, splitFlagIndex + 2 );

                if ( requestLineDatas.size() != 3 )
                {
//                    qDebug() << "JQHttpServer::Session::inspectionBuffer: error2";
                    this->deleteLater();
                    return;
                }

                requestMethod_ = requestLineDatas.at( 0 );
                requestUrl_ = requestLineDatas.at( 1 );
                requestCrlf_ = requestLineDatas.at( 2 );

                if ( ( requestMethod_ != "GET" ) &&
                     ( requestMethod_ != "OPTIONS" ) &&
                     ( requestMethod_ != "POST" ) &&
                     ( requestMethod_ != "PUT" ) )
                {
//                    qDebug() << "JQHttpServer::Session::inspectionBuffer: error3:" << requestMethod_;
                    this->deleteLater();
                    return;
                }
            }
            else if ( splitFlagIndex == 0 )
            {
                receiveBuffer_.remove( 0, 2 );

                headerAcceptedFinished_ = true;

                if ( ( requestMethod_.toUpper() == "GET" ) ||
                     ( requestMethod_.toUpper() == "OPTIONS" ) ||
                     ( ( requestMethod_.toUpper() == "POST" ) && ( ( contentLength_ > 0 ) ? ( !receiveBuffer_.isEmpty() ) : ( true ) ) ) ||
                     ( ( requestMethod_.toUpper() == "PUT" ) && ( ( contentLength_ > 0 ) ? ( !receiveBuffer_.isEmpty() ) : ( true ) ) ) )
                {
                    this->analyseBufferSetup2();
                }
            }
            else
            {
                auto index = receiveBuffer_.indexOf( ':' );

                if ( index <= 0 )
                {
//                    qDebug() << "JQHttpServer::Session::inspectionBuffer: error4";
                    this->deleteLater();
                    return;
                }

                auto headerData = receiveBuffer_.mid( 0, splitFlagIndex );
                receiveBuffer_.remove( 0, splitFlagIndex + 2 );

                const auto &&key = headerData.mid( 0, index );
                auto value = headerData.mid( index + 1 );

                if ( value.startsWith( ' ' ) )
                {
                    value.remove( 0, 1 );
                }

                requestHeader_[ key ] = value;

                if ( key.toLower() == "content-length" )
                {
                    contentLength_ = value.toLongLong();
                }
            }
        }
    }
    else
    {
        this->analyseBufferSetup2();
    }
}

void JQHttpServer::Session::analyseBufferSetup2()
{
    requestBody_ += receiveBuffer_;
    receiveBuffer_.clear();

    if ( !handleAcceptedCallback_ )
    {
        qDebug() << "JQHttpServer::Session::inspectionBuffer: error4";
        this->deleteLater();
        return;
    }

    if ( ( contentLength_ != -1 ) && ( requestBody_.size() != contentLength_ ) )
    {
        return;
    }

    if ( contentAcceptedFinished_ )
    {
        return;
    }

    contentAcceptedFinished_ = true;
    handleAcceptedCallback_( this );
}

void JQHttpServer::Session::onBytesWritten(const qint64 &written)
{
    if ( this->waitWrittenByteCount_ < 0 ) { return; }

    autoCloseTimer_->stop();

    this->waitWrittenByteCount_ -= written;

    if ( this->waitWrittenByteCount_ <= 0 )
    {
        this->waitWrittenByteCount_ = 0;
        socket_->disconnectFromHost();
        return;
    }

    if ( !replyIoDevice_.isNull() )
    {
        if ( replyIoDevice_->atEnd() )
        {
            replyIoDevice_->deleteLater();
            replyIoDevice_.clear();
        }
        else
        {
            if ( requestSourceIp_ == "127.0.0.1" )
            {
                socket_->write( replyIoDevice_->read( 1024 * 1024 ) );
            }
            else
            {
                socket_->write( replyIoDevice_->read( 256 * 1024 ) );
            }
        }
    }

    autoCloseTimer_->start();
}

void JQHttpServer::Session::onStateChanged(const QAbstractSocket::SocketState &socketState)
{
    if ( socketState == QAbstractSocket::UnconnectedState )
    {
        QTimer::singleShot( 1000, this, &QObject::deleteLater );
    }
}

// AbstractManage
JQHttpServer::AbstractManage::AbstractManage(const int &handleMaxThreadCount)
{
    handleThreadPool_.reset( new QThreadPool );
    serverThreadPool_.reset( new QThreadPool );

    handleThreadPool_->setMaxThreadCount( handleMaxThreadCount );
    serverThreadPool_->setMaxThreadCount( 1 );
}

JQHttpServer::AbstractManage::~AbstractManage()
{
    this->stopHandleThread();
}

bool JQHttpServer::AbstractManage::initialize()
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

void JQHttpServer::AbstractManage::deinitialize()
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

bool JQHttpServer::AbstractManage::startServerThread()
{
    QSemaphore semaphore;

    QtConcurrent::run( serverThreadPool_.data(), [ &semaphore, this ]()
    {
        QEventLoop eventLoop;
        QObject::connect( this, &AbstractManage::readyToClose, &eventLoop, &QEventLoop::quit );

        if ( !this->onStart() )
        {
            semaphore.release( 1 );
            return;
        }

        semaphore.release( 1 );

        eventLoop.exec();

        this->onFinish();
    } );

    semaphore.acquire( 1 );

    return this->isRunning();
}

void JQHttpServer::AbstractManage::stopHandleThread()
{
    handleThreadPool_->waitForDone();
}

void JQHttpServer::AbstractManage::stopServerThread()
{
    serverThreadPool_->waitForDone();
}

void JQHttpServer::AbstractManage::newSession(const QPointer< Session > &session)
{
    session->setHandleAcceptedCallback( [ this ](const QPointer< JQHttpServer::Session > &session){ this->handleAccepted( session ); } );

    auto session_ = session.data();
    connect( session.data(), &QObject::destroyed, [ this, session_ ]()
    {
        this->mutex_.lock();
        this->availableSessions_.remove( session_ );
        this->mutex_.unlock();
    } );
    availableSessions_.insert( session.data() );
}

void JQHttpServer::AbstractManage::handleAccepted(const QPointer< Session > &session)
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
JQHttpServer::TcpServerManage::TcpServerManage(const int &handleMaxThreadCount):
    AbstractManage( handleMaxThreadCount )
{ }

JQHttpServer::TcpServerManage::~TcpServerManage()
{
    if ( this->isRunning() )
    {
        this->deinitialize();
    }
}

bool JQHttpServer::TcpServerManage::listen(const QHostAddress &address, const quint16 &port)
{
    listenAddress_ = address;
    listenPort_ = port;

    return this->initialize();
}

bool JQHttpServer::TcpServerManage::isRunning()
{
    return !tcpServer_.isNull();
}

bool JQHttpServer::TcpServerManage::onStart()
{
    mutex_.lock();

    tcpServer_ = new QTcpServer;

    mutex_.unlock();

    QObject::connect( tcpServer_.data(), &QTcpServer::newConnection, [ this ]()
    {
        auto socket = this->tcpServer_->nextPendingConnection();

        this->newSession( new Session( socket ) );
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

void JQHttpServer::TcpServerManage::onFinish()
{
    this->mutex_.lock();

    tcpServer_->close();
    delete tcpServer_.data();
    tcpServer_.clear();

    this->mutex_.unlock();
}

// SslServerManage
#ifndef QT_NO_SSL
namespace JQHttpServer
{

class SslServerHelper: public QTcpServer
{
    void incomingConnection(qintptr socketDescriptor) final;

public:
    std::function< void(qintptr socketDescriptor) > onIncomingConnectionCallback_;
};

void JQHttpServer::SslServerHelper::incomingConnection(qintptr socketDescriptor)
{
    onIncomingConnectionCallback_( socketDescriptor );
}

}

JQHttpServer::SslServerManage::SslServerManage(const int &handleMaxThreadCount):
    AbstractManage( handleMaxThreadCount )
{ }

JQHttpServer::SslServerManage::~SslServerManage()
{
    if ( this->isRunning() )
    {
        this->deinitialize();
    }
}

bool JQHttpServer::SslServerManage::listen(
        const QHostAddress &address,
        const quint16 &port,
        const QString &crtFilePath,
        const QString &keyFilePath,
        const QList< QPair< QString, QSsl::EncodingFormat > > &caFileList,
        const QSslSocket::PeerVerifyMode &peerVerifyMode
    )
{
    listenAddress_ = address;
    listenPort_ = port;

    QFile fileForCrt( crtFilePath );
    if ( !fileForCrt.open( QIODevice::ReadOnly ) )
    {
        qDebug() << "SslServerManage::listen: error: can not open file:" << crtFilePath;
        return false;
    }

    QFile fileForKey( keyFilePath );
    if ( !fileForKey.open( QIODevice::ReadOnly ) )
    {
        qDebug() << "SslServerManage::listen: error: can not open file:" << keyFilePath;
        return false;
    }

    QSslCertificate sslCertificate( fileForCrt.readAll(), QSsl::Pem );
    QSslKey sslKey( fileForKey.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey );

    QList< QSslCertificate > caCertificates;
    for ( const auto &caFile: caFileList )
    {
        QFile fileForCa( caFile.first );
        if ( !fileForCa.open( QIODevice::ReadOnly ) )
        {
            qDebug() << "SslServerManage::listen: error: can not open file:" << caFile.first;
            return false;
        }

        caCertificates.push_back( QSslCertificate( fileForCa.readAll(), caFile.second ) );
    }

    sslConfiguration_.reset( new QSslConfiguration );
    sslConfiguration_->setPeerVerifyMode( peerVerifyMode );
    sslConfiguration_->setPeerVerifyDepth( 1 );
    sslConfiguration_->setLocalCertificate( sslCertificate );
    sslConfiguration_->setPrivateKey( sslKey );
    sslConfiguration_->setProtocol( QSsl::TlsV1_1OrLater );
    sslConfiguration_->setCaCertificates( caCertificates );

    return this->initialize();
}

bool JQHttpServer::SslServerManage::isRunning()
{
    return !tcpServer_.isNull();
}

bool JQHttpServer::SslServerManage::onStart()
{
    mutex_.lock();

    tcpServer_ = new SslServerHelper;

    mutex_.unlock();

    tcpServer_->onIncomingConnectionCallback_ = [ this ](qintptr socketDescriptor)
    {
//        qDebug() << "incomming";

        auto sslSocket = new QSslSocket;

        sslSocket->setSslConfiguration( *sslConfiguration_ );

        QObject::connect( sslSocket, &QSslSocket::encrypted, [ this, sslSocket ]()
        {
            this->newSession( new Session( sslSocket ) );
        } );

//        QObject::connect( sslSocket, static_cast< void(QSslSocket::*)(const QList<QSslError> &errors) >(&QSslSocket::sslErrors), [](const QList<QSslError> &errors)
//        {
//            qDebug() << "sslErrors:" << errors;
//        } );

        sslSocket->setSocketDescriptor( socketDescriptor );
        sslSocket->startServerEncryption();
    };

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

void JQHttpServer::SslServerManage::onFinish()
{
    this->mutex_.lock();

    tcpServer_->close();
    delete tcpServer_.data();
    tcpServer_.clear();

    this->mutex_.unlock();
}

// Service
QSharedPointer< JQHttpServer::Service > JQHttpServer::Service::createService(const QMap< ServiceConfigEnum, QVariant > &config)
{
    QSharedPointer< JQHttpServer::Service > result( new JQHttpServer::Service );

    if ( !result->initialize( config ) ) { return { }; }

    return result;
}

void JQHttpServer::Service::registerProcessor( const QPointer< QObject > &processor )
{
    static QSet< QString > exceptionSlots( { "deleteLater", "_q_reregisterTimers" } );
    static QSet< QString > allowMethod( { "GET", "POST", "DELETE", "PUT" } );

    QString apiPathPrefix;
    for ( auto index = 0; index < processor->metaObject()->classInfoCount(); ++index )
    {
        if ( QString( processor->metaObject()->classInfo( 0 ).name() ) == "apiPathPrefix" )
        {
            apiPathPrefix = processor->metaObject()->classInfo( 0 ).value();
        }
    }

    for ( auto index = 0; index < processor->metaObject()->methodCount(); ++index )
    {
        const auto &&metaMethod = processor->metaObject()->method( index );
        if ( metaMethod.methodType() != QMetaMethod::Slot ) { continue; }

        ApiConfig api;

        api.processor = processor;

        if ( metaMethod.name() == "sessionAccepted" )
        {
            schedules2_[ apiPathPrefix ] =
                [ = ]( const QPointer< JQHttpServer::Session > &session ) {
                    QMetaObject::invokeMethod(
                        processor,
                        "sessionAccepted",
                        Qt::DirectConnection,
                        Q_ARG( QPointer<JQHttpServer::Session>, session ) );
                };

            continue;
        }
        else if ( metaMethod.parameterTypes() == QList< QByteArray >( { "QPointer<JQHttpServer::Session>" } ) )
        {
            api.receiveDataType = NoReceiveDataType;
        }
        else if ( metaMethod.parameterTypes() == QList< QByteArray >( { "QVariantList", "QPointer<JQHttpServer::Session>" } ) )
        {
            api.receiveDataType = VariantListReceiveDataType;
        }
        else if ( metaMethod.parameterTypes() == QList< QByteArray >( { "QVariantMap", "QPointer<JQHttpServer::Session>" } ) )
        {
            api.receiveDataType = VariantMapReceiveDataType;
        }
        else if ( metaMethod.parameterTypes() == QList< QByteArray >( { "QList<QVariantMap>", "QPointer<JQHttpServer::Session>" } ) )
        {
            api.receiveDataType = ListVariantMapReceiveDataType;
        }
        else if ( metaMethod.name() == "certificateVerifier" )
        {
            certificateVerifier_ = processor;
        }
        else
        {
            continue;
        }

        api.slotName = QString( metaMethod.name() );
        if ( exceptionSlots.contains( api.slotName ) ) { continue; }

        for ( const auto &methdo: qAsConst( allowMethod ) )
        {
            if ( api.slotName.startsWith( methdo.toLower() ) )
            {
                api.apiMethod = methdo;
                break;
            }
        }
        if ( api.apiMethod.isEmpty() ) { continue; }

        api.apiName = api.slotName.mid( api.apiMethod.size() );
        if ( api.apiName.isEmpty() ) { continue; }

        {
            auto apiName = api.apiName;

            apiName.push_front( "/" );

            if ( !apiPathPrefix.isEmpty() )
            {
                apiName = apiPathPrefix + apiName;
            }

            schedules_[ api.apiMethod.toUpper() ][ apiName ] = api;
        }

        {
            auto apiName = api.apiName;

            apiName[ 0 ] = apiName[ 0 ].toLower();
            apiName.push_front( "/" );

            if ( !apiPathPrefix.isEmpty() )
            {
                apiName = apiPathPrefix + apiName;
            }

            schedules_[ api.apiMethod.toUpper() ][ apiName ] = api;
        }
    }
}

QJsonDocument JQHttpServer::Service::extractPostJsonData(const QPointer< JQHttpServer::Session > &session)
{
    return QJsonDocument::fromJson( session->requestBody() );
}

void JQHttpServer::Service::reply(
    const QPointer< JQHttpServer::Session > &session,
    const QJsonObject &data,
    const bool &isSucceed,
    const QString &message,
    const int &httpStatusCode )
{
    QJsonObject result;
    result[ "isSucceed" ] = isSucceed;
    result[ "message" ] = message;

    if ( !data.isEmpty() )
    {
        result[ "data" ] = data;
    }

    session->replyJsonObject( result, httpStatusCode );
}

void JQHttpServer::Service::reply(
    const QPointer< JQHttpServer::Session > &session,
    const bool &isSucceed,
    const QString &message,
    const int &httpStatusCode )
{
    reply( session, QJsonObject(), isSucceed, message, httpStatusCode );
}

void JQHttpServer::Service::httpGetPing(const QPointer< JQHttpServer::Session > &session)
{
    QJsonObject data;
    data[ "serverTime" ] = QDateTime::currentMSecsSinceEpoch();

    reply( session, data );
}

void JQHttpServer::Service::httpGetFaviconIco(const QPointer< JQHttpServer::Session > &session)
{
    QImage image( 256, 256, QImage::Format_ARGB32 );
    image.fill( 0x0 );

    QPainter painter( &image );
    painter.setPen( Qt::NoPen );
    painter.setBrush( QColor( "#ff00ff" ) );
    painter.drawEllipse( 16, 16, 224, 224 );
    painter.end();

    session->replyImage( image );
}

void JQHttpServer::Service::httpOptions(const QPointer< JQHttpServer::Session > &session)
{
    session->replyOptions();
}

bool JQHttpServer::Service::initialize( const QMap< JQHttpServer::ServiceConfigEnum, QVariant > &config )
{
    if ( config.contains( ServiceProcessor ) &&
         config[ ServiceProcessor ].canConvert< QPointer< QObject > >() &&
         !config[ ServiceProcessor ].value< QPointer< QObject > >().isNull() )
    {
        this->registerProcessor( config[ ServiceProcessor ].value< QPointer< QObject > >() );
    }

    if ( config.contains( ServiceProcessor ) &&
         config[ ServiceProcessor ].canConvert< QList< QPointer< QObject > > >() )
    {
        for ( const auto &process: config[ ServiceProcessor ].value< QList< QPointer< QObject > > >() )
        {
            if ( !process ) { continue; }

            this->registerProcessor( process );
        }
    }

    const auto httpPort = static_cast< quint16 >( config[ ServiceHttpListenPort ].toInt() );
    if ( httpPort > 0 )
    {
        this->httpServerManage_.reset( new JQHttpServer::TcpServerManage );
        this->httpServerManage_->setHttpAcceptedCallback( std::bind( &JQHttpServer::Service::onSessionAccepted, this, std::placeholders::_1 ) );

        if ( !this->httpServerManage_->listen(
                 QHostAddress::Any,
                 httpPort
             ) )
        {
            qWarning() << "JQHttpServer::Service: listen port error:" << httpPort;
            return false;
        }
    }

    const auto httpsPort = static_cast< quint16 >( config[ ServiceHttpsListenPort ].toInt() );
    if ( httpsPort > 0 )
    {
        this->httpsServerManage_.reset( new JQHttpServer::SslServerManage );
        this->httpsServerManage_->setHttpAcceptedCallback( std::bind( &JQHttpServer::Service::onSessionAccepted, this, std::placeholders::_1 ) );

        auto peerVerifyMode = QSslSocket::VerifyNone;
        if ( config.contains( ServiceSslPeerVerifyMode ) )
        {
            peerVerifyMode = static_cast< QSslSocket::PeerVerifyMode >( config[ ServiceSslPeerVerifyMode ].toInt() );
        }

        QString crtFilePath = config[ ServiceSslCrtFilePath ].toString();
        QString keyFilePath = config[ ServiceSslKeyFilePath ].toString();
        if ( crtFilePath.isEmpty() || keyFilePath.isEmpty() )
        {
            qWarning() << "JQHttpServer::Service: crt or key file path error";
            return false;
        }

        QList< QPair< QString, QSsl::EncodingFormat > > caFileList;
        for ( const auto &caFilePath: config[ ServiceSslCAFilePath ].toStringList() )
        {
            QPair< QString, QSsl::EncodingFormat > pair;
            pair.first = caFilePath;
            pair.second = QSsl::Pem;
            caFileList.push_back( pair );
        }

        if ( !this->httpsServerManage_->listen(
                 QHostAddress::Any,
                 httpsPort,
                 crtFilePath,
                 keyFilePath,
                 caFileList,
                 peerVerifyMode
             ) )
        {
            qWarning() << "JQHttpServer::Service: listen port error:" << httpsPort;
            return false;
        }
    }

    const auto serviceUuid = config[ ServiceUuid ].toString();
    if ( !QUuid( serviceUuid ).isNull() )
    {
        this->serviceUuid_ = serviceUuid;
    }

    return true;
}

void JQHttpServer::Service::onSessionAccepted(const QPointer< JQHttpServer::Session > &session)
{
    if ( certificateVerifier_ && qobject_cast< QSslSocket * >( session->socket() ) )
    {
        QMetaObject::invokeMethod(
                    certificateVerifier_,
                    "certificateVerifier",
                    Qt::DirectConnection,
                    Q_ARG( QSslCertificate, session->peerCertificate() ),
                    Q_ARG( QPointer<JQHttpServer::Session>, session )
                );

        if ( session->replyHttpCode() >= 0 ) { return; }
    }

    const auto schedulesIt = schedules_.find( session->requestMethod() );
    if ( schedulesIt != schedules_.end() )
    {
        auto apiName = session->requestUrlPath();

        auto it = schedulesIt.value().find( session->requestUrlPath() );
        if ( ( it == schedulesIt.value().end() ) && ( session->requestUrlPath().contains( "_" ) ) )
        {
            apiName = Service::snakeCaseToCamelCase( session->requestUrlPath() );
            it = schedulesIt.value().find( apiName );
        }

        if ( it != schedulesIt.value().end() )
        {
            Recoder recoder( session );
            recoder.serviceUuid_ = serviceUuid_;
            recoder.apiName = apiName;

            switch ( it->receiveDataType )
            {
                case NoReceiveDataType:
                {
                    QMetaObject::invokeMethod(
                                it->processor,
                                it->slotName.toLatin1().data(),
                                Qt::DirectConnection,
                                Q_ARG( QPointer<JQHttpServer::Session>, session )
                            );
                    return;
                }
                case VariantListReceiveDataType:
                {
                    const auto &&json = this->extractPostJsonData( session );
                    if ( !json.isNull() )
                    {
                        QMetaObject::invokeMethod(
                                    it->processor,
                                    it->slotName.toLatin1().data(),
                                    Qt::DirectConnection,
                                    Q_ARG( QVariantList, json.array().toVariantList() ),
                                    Q_ARG( QPointer<JQHttpServer::Session>, session )
                                );
                        return;
                    }
                    break;
                }
                case VariantMapReceiveDataType:
                {
                    const auto &&json = this->extractPostJsonData( session );
                    if ( !json.isNull() )
                    {
                        QMetaObject::invokeMethod(
                                    it->processor,
                                    it->slotName.toLatin1().data(),
                                    Qt::DirectConnection,
                                    Q_ARG( QVariantMap, json.object().toVariantMap() ),
                                    Q_ARG( QPointer<JQHttpServer::Session>, session )
                                );
                        return;
                    }
                    break;
                }
                case ListVariantMapReceiveDataType:
                {
                    const auto &&json = this->extractPostJsonData( session );
                    if ( !json.isNull() )
                    {
                        QMetaObject::invokeMethod(
                                    it->processor,
                                    it->slotName.toLatin1().data(),
                                    Qt::DirectConnection,
                                    Q_ARG( QList<QVariantMap>, Service::variantListToListVariantMap( json.array().toVariantList() ) ),
                                    Q_ARG( QPointer<JQHttpServer::Session>, session )
                                );
                        return;
                    }
                    break;
                }
                default:
                {
                    qDebug() << "onSessionAccepted: data type not match:" << it->receiveDataType;
                    reply( session, false, "data type not match", 404 );
                    return;
                }
            }

            qDebug() << "onSessionAccepted: data error:" << it->receiveDataType;
            reply( session, false, "data error", 404 );
            return;
        }
    }

    for ( auto it = schedules2_.begin(); it != schedules2_.end(); ++it )
    {
        if ( session->requestUrlPath().startsWith( it.key() ) )
        {
            it.value()( session );
            return;
        }
    }

    if ( ( session->requestMethod() == "GET" ) && ( session->requestUrlPath() == "/ping" ) )
    {
        this->httpGetPing( session );
        return;
    }
    else if ( ( session->requestMethod() == "GET" ) && ( session->requestUrlPath() == "/favicon.ico" ) )
    {
        this->httpGetFaviconIco( session );
        return;
    }
    else if ( session->requestMethod() == "OPTIONS" )
    {
        this->httpOptions( session );
        return;
    }

    qDebug().noquote() << "API not found:" << session->requestMethod() << session->requestUrlPath();
    reply( session, false, "API not found", 404 );
}

QString JQHttpServer::Service::snakeCaseToCamelCase(const QString &source, const bool &firstCharUpper)
{
#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 15, 0 ) )
    const auto &&splitList = source.split( '_', Qt::SkipEmptyParts );
#else
    const auto &&splitList = source.split( '_', QString::SkipEmptyParts );
#endif

    QString result;

    for ( const auto &splitTag: splitList )
    {
        if ( splitTag.size() == 1 )
        {
            if ( result.isEmpty() )
            {
                if ( firstCharUpper )
                {
                    result += splitTag[ 0 ].toUpper();
                }
                else
                {
                    result += splitTag;
                }
            }
            else
            {
                result += splitTag[ 0 ].toUpper();
            }
        }
        else
        {
            if ( result.isEmpty() )
            {
                if ( firstCharUpper )
                {
                    result += splitTag[ 0 ].toUpper();
                    result += std::move( splitTag.mid( 1 ) );
                }
                else
                {
                    result += splitTag;
                }
            }
            else
            {
                result += splitTag[ 0 ].toUpper();
                result += std::move( splitTag.mid( 1 ) );
            }
        }
    }

    return result;
}

QList< QVariantMap > JQHttpServer::Service::variantListToListVariantMap(const QVariantList &source)
{
    QList< QVariantMap > result;

    for ( const auto &item: source )
    {
        result.push_back( item.toMap() );
    }

    return result;
}

JQHttpServer::Service::Recoder::Recoder(const QPointer< JQHttpServer::Session > &session)
{
    qDebug() << "HTTP accepted:" << session->requestMethod().toLatin1().data() << session->requestUrlPath().toLatin1().data();

    session_ = session;
    acceptedTime_ = QDateTime::currentDateTime();
}

JQHttpServer::Service::Recoder::~Recoder()
{
    if ( !session_ )
    {
        return;
    }

    const auto &&replyTime = QDateTime::currentDateTime();
    const auto &&elapsed = replyTime.toMSecsSinceEpoch() - acceptedTime_.toMSecsSinceEpoch();

    qDebug().noquote() << "HTTP finished:" << QString::number( elapsed ).rightJustified( 3, ' ' )
                       << "ms, code:" << session_->replyHttpCode()
                       << ", accepted:" << QString::number( session_->requestBody().size() ).rightJustified( 3, ' ' )
                       << ", reply:" << QString::number( session_->replyBodySize() ).rightJustified( 3, ' ' );
}
#endif
