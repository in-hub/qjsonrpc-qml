#include <QLocalServer>
#include <QLocalSocket>

#include <QtCore/QEventLoop>
#include <QtCore/QVariant>
#include <QtTest/QtTest>

#include "json/qjsondocument.h"
#include "qjsonrpcservice.h"
#include "qjsonrpcmessage.h"

class TestQJsonRpcService: public QObject
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
    void testServiceProviderNoParameter();
    void testServiceProviderSingleParameter();
    void testServiceProviderMultiparameter();
    void testServiceProviderInvalidArgs();
    void testServiceProviderMethodNotFound();
};

class TestService : public QJsonRpcService
{
    Q_OBJECT
public:
    TestService(QObject *parent = 0)
        : QJsonRpcService(parent)
    {
    }

    ~TestService()
    {
    }

    QString serviceName() const
    {
        return QString("service");
    }

public Q_SLOTS:
    void noParam() const
    {
    }

    QString singleParam(const QString &string) const
    {
        return string;
    }

    QString multipleParam(const QString &first,
                          const QString &second,
                          const QString &third) const
    {
        return first + second + third;
    }
};

void TestQJsonRpcService::initTestCase()
{
    qRegisterMetaType<QJsonRpcMessage>("QJsonRpcMessage");
}

void TestQJsonRpcService::cleanupTestCase()
{
}

void TestQJsonRpcService::init()
{
}

void TestQJsonRpcService::cleanup()
{
}

void TestQJsonRpcService::testSocketNoParameters()
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

void TestQJsonRpcService::testSocketMultiparamter()
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

void TestQJsonRpcService::testSocketNotification()
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

void TestQJsonRpcService::testSocketResponse()
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

void TestQJsonRpcService::testServiceProviderNoParameter()
{
    // Initialize the service provider.
    QEventLoop loop;
    TestService service;
    QLocalServer localServer;
    QVERIFY(localServer.listen("test"));
    QJsonRpcServiceProvider serviceProvider(&localServer);
    serviceProvider.addService(&service);

    // Connect to the socket.
    QLocalSocket socket;
    socket.connectToServer("test");
    QJsonRpcServiceSocket serviceSocket(&socket, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));

    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.noParam");
    QJsonRpcServiceReply *reply = serviceSocket.sendMessage(request);
    connect(&serviceSocket, SIGNAL(messageReceived(QJsonRpcMessage)), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QJsonRpcMessage response = reply->response();
    QCOMPARE(spyMessageReceived.count(), 0);
    QVERIFY(response.errorCode() == 0);
    QCOMPARE(request.id(), response.id());
}

void TestQJsonRpcService::testServiceProviderSingleParameter()
{
    // Initialize the service provider.
    QEventLoop loop;
    TestService service;
    QLocalServer localServer;
    QVERIFY(localServer.listen("test"));
    QJsonRpcServiceProvider serviceProvider(&localServer);
    serviceProvider.addService(&service);

    // Connect to the socket.
    QLocalSocket socket;
    socket.connectToServer("test");
    QJsonRpcServiceSocket serviceSocket(&socket, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));

    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.singleParam", QString("single"));
    QJsonRpcServiceReply *reply = serviceSocket.sendMessage(request);
    connect(&serviceSocket, SIGNAL(messageReceived(QJsonRpcMessage)), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QJsonRpcMessage response = reply->response();
    QCOMPARE(spyMessageReceived.count(), 0);
    QVERIFY(response.errorCode() == 0);
    QCOMPARE(request.id(), response.id());
    QVERIFY(response.result() == QString("single"));
}

void TestQJsonRpcService::testServiceProviderMultiparameter()
{
    // Initialize the service provider.
    QEventLoop loop;
    TestService service;
    QLocalServer localServer;
    QVERIFY(localServer.listen("test"));
    QJsonRpcServiceProvider serviceProvider(&localServer);
    serviceProvider.addService(&service);

    // Connect to the socket.
    QLocalSocket socket;
    socket.connectToServer("test");
    QJsonRpcServiceSocket serviceSocket(&socket, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));

    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.multipleParam",
                                                             QVariantList() << QVariant(QString("a"))
                                                                            << QVariant(QString("b"))
                                                                            << QVariant(QString("c")));
    QJsonRpcServiceReply *reply = serviceSocket.sendMessage(request);
    connect(&serviceSocket, SIGNAL(messageReceived(QJsonRpcMessage)), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QJsonRpcMessage response = reply->response();
    QCOMPARE(spyMessageReceived.count(), 0);
    QVERIFY(response.errorCode() == 0);
    QCOMPARE(request.id(), response.id());
    QVERIFY(response.result() == QString("abc"));
}

void TestQJsonRpcService::testServiceProviderInvalidArgs()
{
    // Initialize the service provider.
    QEventLoop loop;
    TestService service;
    QLocalServer localServer;
    QVERIFY(localServer.listen("test"));
    QJsonRpcServiceProvider serviceProvider(&localServer);
    serviceProvider.addService(&service);

    // Connect to the socket.
    QLocalSocket socket;
    socket.connectToServer("test");
    QJsonRpcServiceSocket serviceSocket(&socket, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));

    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.noParam",
                                                             QVariantList() << false);
    QJsonRpcServiceReply *reply = serviceSocket.sendMessage(request);
    connect(&serviceSocket, SIGNAL(messageReceived(QJsonRpcMessage)), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QCOMPARE(spyMessageReceived.count(), 1);
    if (spyMessageReceived.count() == 1) {
        QVariant message = spyMessageReceived.takeFirst().at(0);
        QJsonRpcMessage error = message.value<QJsonRpcMessage>();
        QCOMPARE(request.id(), error.id());
        QVERIFY(error.errorCode() == QJsonRpc::InvalidParams);
    }
}

void TestQJsonRpcService::testServiceProviderMethodNotFound()
{
    // Initialize the service provider.
    QEventLoop loop;
    TestService service;
    QLocalServer localServer;
    QVERIFY(localServer.listen("test"));
    QJsonRpcServiceProvider serviceProvider(&localServer);
    serviceProvider.addService(&service);

    // Connect to the socket.
    QLocalSocket socket;
    socket.connectToServer("test");
    QJsonRpcServiceSocket serviceSocket(&socket, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));

    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.doesNotExist");
    QJsonRpcServiceReply *reply = serviceSocket.sendMessage(request);
    connect(&serviceSocket, SIGNAL(messageReceived(QJsonRpcMessage)), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QCOMPARE(spyMessageReceived.count(), 1);
    if (spyMessageReceived.count() == 1) {
        QVariant message = spyMessageReceived.takeFirst().at(0);
        QJsonRpcMessage error = message.value<QJsonRpcMessage>();
        QCOMPARE(request.id(), error.id());
        QVERIFY(error.errorCode() == QJsonRpc::MethodNotFound);
    }
}

QTEST_MAIN(TestQJsonRpcService)
#include "tst_qjsonrpcservice.moc"
