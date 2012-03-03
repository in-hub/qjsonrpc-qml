#include <QtCore/QVariant>
#include <QtTest/QtTest>

#include "json/qjsondocument.h"
#include "qjsonrpcpeer.h"

class TestQJsonRpcPeer: public QObject
{
    Q_OBJECT  
private slots:
    void testFail();
};

void TestQJsonRpcPeer::testFail()
{
    QCOMPARE(1, 0);
}

QTEST_MAIN(TestQJsonRpcPeer)
#include "tst_qjsonrpcpeer.moc"
