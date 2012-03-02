#include <QtCore/QVariant>
#include <QtTest/QtTest>

#include "json/qjsondocument.h"
#include "qjsonrpc.h"

class TestQJsonRpcMessage: public QObject
{
    Q_OBJECT  
private slots:
    void testInvalidData();
    void testResponseSameId();
    void testNotificationNoId();
    void testMessageTypes();
    void testPositionalParameters();
};

void TestQJsonRpcMessage::testInvalidData()
{
    QJsonObject invalidData;
    QJsonRpcMessage message(invalidData);
    QCOMPARE(message.type(), QJsonRpcMessage::Invalid);
}

void TestQJsonRpcMessage::testResponseSameId()
{
    QJsonRpcMessage request = QJsonRpcMessage::createRequest("testMethod");
    QJsonRpcMessage response = request.createResponse("testResponse");
    QCOMPARE(response.id(), request.id());
}

void TestQJsonRpcMessage::testNotificationNoId()
{
    QJsonRpcMessage notification = QJsonRpcMessage::createNotification("testNotification");
    QCOMPARE(notification.id(), -1);
}

void TestQJsonRpcMessage::testMessageTypes()
{
    QJsonRpcMessage invalid;
    QCOMPARE(invalid.type(), QJsonRpcMessage::Invalid);

    QJsonRpcMessage request = QJsonRpcMessage::createRequest("testMethod");
    QCOMPARE(request.type(), QJsonRpcMessage::Request);

    QJsonRpcMessage response = request.createResponse("testResponse");
    QCOMPARE(response.type(), QJsonRpcMessage::Response);

    QJsonRpcMessage error = request.createErrorResponse(QJsonRpc::NoError);
    QCOMPARE(error.type(), QJsonRpcMessage::Error);

    QJsonRpcMessage notification = QJsonRpcMessage::createNotification("testNotification");
    QCOMPARE(notification.type(), QJsonRpcMessage::Notification);
}

// this is from the spec, I don't think it proves much..
void TestQJsonRpcMessage::testPositionalParameters()
{
    const char *first = "{\"jsonrpc\": \"2.0\", \"method\": \"subtract\", \"params\": [42, 23], \"id\": 1}";
    QJsonObject firstObject = QJsonDocument::fromJson(first).object();
    const char *second = "{\"jsonrpc\": \"2.0\", \"method\": \"subtract\", \"params\": [23, 42], \"id\": 2}";
    QJsonObject secondObject = QJsonDocument::fromJson(second).object();
    QVERIFY2(firstObject.value("params").toArray() != secondObject.value("params").toArray(), "params should maintain order");
}



QTEST_MAIN(TestQJsonRpcMessage)
#include "tst_qjsonrpcmessage.moc"
