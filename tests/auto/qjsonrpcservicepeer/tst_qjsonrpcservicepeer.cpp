#include <QLocalSocket>
#include <QTcpSocket>

#include <QtCore/QEventLoop>
#include <QtCore/QVariant>
#include <QtTest/QtTest>

#include "json/qjsondocument.h"
#include "qjsonrpcservice.h"
#include "qjsonrpcmessage.h"

class TestQJsonRpcServicePeer : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Local Server
    void testLocalPeerBasic();

};

class TestPeerService : public QJsonRpcService
{
    Q_OBJECT
    Q_CLASSINFO("serviceName", "service")
public:
    TestPeerService(QObject *parent = 0)
        : QJsonRpcService(parent),
          m_called(0)
    {}

    int called() const {
        return m_called;
    }

public Q_SLOTS:
    void testMethod() {
        m_called++;
    }

private:
    int m_called;

};

void TestQJsonRpcServicePeer::initTestCase()
{
}

void TestQJsonRpcServicePeer::cleanupTestCase()
{
}

void TestQJsonRpcServicePeer::init()
{
}

void TestQJsonRpcServicePeer::cleanup()
{
}

void TestQJsonRpcServicePeer::testLocalPeerBasic()
{
    QJsonRpcLocalServicePeer firstPeer;
    TestPeerService firstPeerService;
    firstPeerService.setObjectName("firstPeerService");
    firstPeer.addService(&firstPeerService);
    QVERIFY(firstPeer.listen("test"));

    QJsonRpcLocalServicePeer secondPeer;
    TestPeerService secondPeerService;
    secondPeerService.setObjectName("secondPeerService");
    secondPeer.addService(&secondPeerService);
    QVERIFY(secondPeer.connectToPeer("test"));

    QCOMPARE(firstPeerService.called(), 0);
    QCOMPARE(secondPeerService.called(), 0);
    QJsonRpcServiceReply *firstReply = secondPeer.invokeRemoteMethod("service.testMethod");
    QEventLoop firstReplyLoop;
    connect(firstReply, SIGNAL(finished()), &firstReplyLoop, SLOT(quit()));
    QTimer::singleShot(50, &firstReplyLoop, SLOT(quit()));
    firstReplyLoop.exec();

    QCOMPARE(firstPeerService.called(), 1);
    QCOMPARE(secondPeerService.called(), 0);

    firstPeer.notifyConnectedClients("service.testMethod");
    QEventLoop secondReplyLoop;
    QTimer::singleShot(50, &secondReplyLoop, SLOT(quit()));
    secondReplyLoop.exec();

    QCOMPARE(firstPeerService.called(), 1);
    QCOMPARE(secondPeerService.called(), 1);
}

QTEST_MAIN(TestQJsonRpcServicePeer)
#include "tst_qjsonrpcservicepeer.moc"
