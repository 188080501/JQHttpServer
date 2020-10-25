#ifndef OVERALLTEST_H_
#define OVERALLTEST_H_

// Qt lib import
#include <QObject>
#include <QSharedPointer>

namespace JQHttpServer
{
class TcpServerManage;
#ifndef QT_NO_SSL
class SslServerManage;
#endif
}

class OverallTest: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY( OverallTest )

public:
    OverallTest() = default;

    ~OverallTest() = default;

private slots:
    void initTestCase();

    void cleanupTestCase();

    void connectTest();

    void httpGetTest();

    void httpPostTest();

#ifndef QT_NO_SSL
    void httpsGetTest();

    void httpsPostTest();
#endif

private:
    QSharedPointer< JQHttpServer::TcpServerManage > httpServerManage_;
#ifndef QT_NO_SSL
    QSharedPointer< JQHttpServer::SslServerManage > httpsServerManage_;
#endif
};

#endif//OVERALLTEST_H_
