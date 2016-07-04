#ifndef __BenchMark_h__
#define __BenchMark_h__

// Qt lib import
#include <QObject>
#include <QSharedPointer>

namespace JQHttpServer
{
class Session;
class Manage;
}

class BenchMark: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void cleanupTestCase();

    void benchMarkForOnce();

    void benchMarkFor5000();

private:
    QSharedPointer< JQHttpServer::Manage > httpServerManage_;
};

#endif//__BenchMark_h__
