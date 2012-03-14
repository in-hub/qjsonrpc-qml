#include <QLocalServer>
#include <QLocalSocket>

#include <QtCore/QEventLoop>
#include <QtCore/QVariant>
#include <QtTest/QtTest>

#include "json/qjsondocument.h"
#include "qjsonrpcservice.h"
#include "qjsonrpcmessage.h"

class TestQJsonRpcServiceSocket: public QObject
{
    Q_OBJECT  
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    void testSocketNoParameters();
    void testSocketMultiparamter();
    void testSocketNotification();
    void testSocketResponse();
};

void TestQJsonRpcServiceSocket::initTestCase()
{
    qRegisterMetaType<QJsonRpcMessage>("QJsonRpcMessage");
}

void TestQJsonRpcServiceSocket::cleanupTestCase()
{
}

void TestQJsonRpcServiceSocket::init()
{
}

void TestQJsonRpcServiceSocket::cleanup()
{
}

void TestQJsonRpcServiceSocket::testSocketNoParameters()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QJsonRpcServiceSocket serviceSocket(&buffer, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));
    QVERIFY(serviceSocket.isValid());

    QJsonRpcMessage request = QJsonRpcMessage::createRequest(QString("test.noParam"));

    QJsonRpcServiceReply *reply;
    reply = serviceSocket.sendMessage(request);

    QJsonDocument document = QJsonDocument::fromJson(buffer.data());
    QVERIFY(!document.isEmpty());
    QJsonRpcMessage bufferMessage(document.object());

    QCOMPARE(request.id(), bufferMessage.id());
    QCOMPARE(request.type(), bufferMessage.type());
    QCOMPARE(request.method(), bufferMessage.method());
    QCOMPARE(request.params(), bufferMessage.params());
    QCOMPARE(spyMessageReceived.count(), 0);
}

void TestQJsonRpcServiceSocket::testSocketMultiparamter()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QJsonRpcServiceSocket serviceSocket(&buffer, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));
    QVERIFY(serviceSocket.isValid());

    QJsonRpcMessage request = QJsonRpcMessage::createRequest(QString("test.multiParam"),
                                                             QVariantList() << false << true);

    QJsonRpcServiceReply *reply;
    reply = serviceSocket.sendMessage(request);

    QJsonDocument document = QJsonDocument::fromJson(buffer.data());
    QVERIFY(!document.isEmpty());
    QJsonRpcMessage bufferMessage(document.object());

    QCOMPARE(request.id(), bufferMessage.id());
    QCOMPARE(request.type(), bufferMessage.type());
    QCOMPARE(request.method(), bufferMessage.method());
    QCOMPARE(request.params(), bufferMessage.params());
    QCOMPARE(spyMessageReceived.count(), 0);
}

void TestQJsonRpcServiceSocket::testSocketNotification()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QJsonRpcServiceSocket serviceSocket(&buffer, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));
    QVERIFY(serviceSocket.isValid());

    QJsonRpcMessage notification = QJsonRpcMessage::createNotification("test.notify");

    QJsonRpcServiceReply *reply;
    reply = serviceSocket.sendMessage(notification);

    QJsonDocument document = QJsonDocument::fromJson(buffer.data());
    QVERIFY(!document.isEmpty());
    QJsonRpcMessage bufferMessage(document.object());

    QCOMPARE(notification.id(), bufferMessage.id());
    QCOMPARE(notification.type(), bufferMessage.type());
    QCOMPARE(notification.method(), bufferMessage.method());
    QCOMPARE(notification.params(), bufferMessage.params());
    QCOMPARE(spyMessageReceived.count(), 0);
}

void TestQJsonRpcServiceSocket::testSocketResponse()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QJsonRpcServiceSocket serviceSocket(&buffer, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));
    QVERIFY(serviceSocket.isValid());

    QJsonRpcMessage response = QJsonRpcMessage::createRequest(QString("test.response"));
    response = response.createResponse(QVariant());

    QJsonRpcServiceReply *reply;
    reply = serviceSocket.sendMessage(response);

    QJsonDocument document = QJsonDocument::fromJson(buffer.data());
    QVERIFY(!document.isEmpty());
    QJsonRpcMessage bufferMessage(document.object());

    QCOMPARE(response.id(), bufferMessage.id());
    QCOMPARE(response.type(), bufferMessage.type());
    QCOMPARE(response.method(), bufferMessage.method());
    QCOMPARE(response.params(), bufferMessage.params());
    QCOMPARE(spyMessageReceived.count(), 0);
}

QTEST_MAIN(TestQJsonRpcServiceSocket)
#include "tst_qjsonrpcservicesocket.moc"
