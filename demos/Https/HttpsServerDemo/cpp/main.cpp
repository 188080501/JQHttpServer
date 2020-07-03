// Qt lib import
#include <QtCore>
#include <QImage>

// JQLibrary import
#include "JQHttpServer.h"

int main(int argc, char *argv[])
{
    QCoreApplication app( argc, argv );

#ifndef QT_NO_SSL
    JQHttpServer::SslServerManage sslServerManage( 2 ); // 设置最大处理线程数，默认2个

    sslServerManage.setHttpAcceptedCallback( []( const QPointer< JQHttpServer::Session > &session )
    {
        // 回调发生在新的线程内，不是主线程，请注意线程安全
        // 若阻塞了此回调，那么新的连接将不会得到处理（默认情况下有2个线程可以阻塞2次，第3个连接将不会被处理）

        session->replyText( QString( "url:%1\ndata:%2" ).arg( session->requestUrl(), QString( session->requestBody() ) ) );
//        session->replyJsonObject( { { { "message", "ok" } } } );
//        session->replyJsonArray( { "a", "b", "c" } );
//        session->replyFile( "/Users/jason/Desktop/Test1.Test2" );
//        session->replyImage( QImage( "/Users/jason/Desktop/Test.png" ) );

        // 注1：因为一个session对应一个单一的HTTP请求，所以session只能reply一次
        // 注2：在reply后，session的生命周期不可控，所以reply后不要再调用session的接口了
    } );

    qDebug() << "listen:" << sslServerManage.listen( QHostAddress::Any, 24684, ":/server.crt", ":/server.key" );

    // 这是我在一个实际项目中用的配置（用认证的证书，访问时带绿色小锁），其中涉及到隐私的细节已经被替换，但是任然能够看出整体用法
//    qDebug() << "listen:" << sslServerManage.listen(
//                    QHostAddress::Any,
//                    24684,
//                    "xxx/__xxx_com.crt",
//                    "xxx/__xxx_com.key",
//                    {
//                        { "xxx/__xxx_com.ca-bundle", QSsl::Pem },
//                        { "xxx/COMODO RSA Certification Authority.crt", QSsl::Pem },
//                        { "xxx/COMODO RSA Domain Validation Secure Server CA.cer", QSsl::Der }
//                    }
//                );
    return app.exec();
#else
    return 0;
#endif
}
