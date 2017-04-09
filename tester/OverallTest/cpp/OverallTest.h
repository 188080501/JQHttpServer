#ifndef __OverallTest_h__
#define __OverallTest_h__

// Qt lib import
#include <QObject>
#include <QSharedPointer>

namespace JQHttpServer
{
class TcpServerManage;
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

private:
    QSharedPointer< JQHttpServer::TcpServerManage > tcpServerManage_;
};

#endif//__OverallTest_h__
