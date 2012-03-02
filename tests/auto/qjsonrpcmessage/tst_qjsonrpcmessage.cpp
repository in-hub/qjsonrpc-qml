#include <QtCore/QVariant>
#include <QtTest/QtTest>

#include "json/qjsondocument.h"
#include "qjsonrpc.h"

class TestQJsonRpcMessage: public QObject
{
    Q_OBJECT  
private slots:
    void testPositionalParameters();
    void testNotification();
};

void TestQJsonRpcMessage::testPositionalParameters()
{
    const char *first = "{\"jsonrpc\": \"2.0\", \"method\": \"subtract\", \"params\": [42, 23], \"id\": 1}";
    QJsonObject firstObject = QJsonDocument::fromJson(first).object();
    const char *second = "{\"jsonrpc\": \"2.0\", \"method\": \"subtract\", \"params\": [23, 42], \"id\": 2}";
    QJsonObject secondObject = QJsonDocument::fromJson(second).object();
    QVERIFY2(firstObject.value("params").toArray() != secondObject.value("params").toArray(), "params should maintain order");
}

void TestQJsonRpcMessage::testNotification()
{
    const char *message = "{\"jsonrpc\": \"2.0\", \"method\": \"update\", \"params\": [1,2,3,4,5]}";
    QJsonObject messageObject = QJsonDocument::fromJson(message).object();
    QJsonRpcMessage test(messageObject);
    QVERIFY(test.type() == QJsonRpcMessage::Notification);
}

QTEST_MAIN(TestQJsonRpcMessage)
#include "tst_qjsonrpcmessage.moc"
