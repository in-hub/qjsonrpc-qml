#include <QLocalServer>
#include <QLocalSocket>
#include <QTcpServer>
#include <QTcpSocket>

#include <QtCore/QEventLoop>
#include <QtCore/QVariant>
#include <QtTest/QtTest>

#include "json/qjsondocument.h"
#include "qjsonrpcservice.h"
#include "qjsonrpcmessage.h"

class TestQJsonRpcServiceProvider: public QObject
{
    Q_OBJECT  
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Local Server
    void testLocalNoParameter();
    void testLocalSingleParameter();
    void testLocalMultiparameter();
    void testLocalInvalidArgs();
    void testLocalMethodNotFound();
    void testLocalNotifyConnectedClients();
    void testLocalNumberParameters();

    // TCP Server
    void testTcpNoParameter();
    void testTcpSingleParameter();
    void testTcpMultiparameter();
    void testTcpInvalidArgs();
    void testTcpMethodNotFound();
    void testTcpNotifyConnectedClients();

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

    void numberParameters(int intParam, double doubleParam, float floatParam)
    {
        Q_UNUSED(intParam)
        Q_UNUSED(doubleParam)
        Q_UNUSED(floatParam)
    }

};

void TestQJsonRpcServiceProvider::initTestCase()
{
    qRegisterMetaType<QJsonRpcMessage>("QJsonRpcMessage");
}

void TestQJsonRpcServiceProvider::cleanupTestCase()
{
}

void TestQJsonRpcServiceProvider::init()
{
}

void TestQJsonRpcServiceProvider::cleanup()
{
}

void TestQJsonRpcServiceProvider::testLocalNoParameter()
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
    QVERIFY(socket.waitForConnected());
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

void TestQJsonRpcServiceProvider::testLocalSingleParameter()
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
    QVERIFY(socket.waitForConnected());
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

void TestQJsonRpcServiceProvider::testLocalMultiparameter()
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
    QVERIFY(socket.waitForConnected());
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

void TestQJsonRpcServiceProvider::testLocalInvalidArgs()
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
    QVERIFY(socket.waitForConnected());
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

void TestQJsonRpcServiceProvider::testLocalMethodNotFound()
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
    QVERIFY(socket.waitForConnected());
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

class ServiceProviderNotificationHelper : public QObject
{
    Q_OBJECT
public:
    ServiceProviderNotificationHelper(const QJsonRpcMessage &message, QJsonRpcServiceProvider *provider)
        : m_provider(provider),
          m_notification(message) {}

public Q_SLOTS:
    void activate() {
        m_provider->notifyConnectedClients(m_notification);
    }

private:
    QJsonRpcServiceProvider *m_provider;
    QJsonRpcMessage m_notification;

};

void TestQJsonRpcServiceProvider::testLocalNotifyConnectedClients()
{
    // Initialize the service provider.
    QEventLoop firstLoop;
    TestService service;
    QLocalServer localServer;
    QVERIFY(localServer.listen("test"));
    QJsonRpcServiceProvider serviceProvider(&localServer);
    serviceProvider.addService(&service);

    // first client
    QLocalSocket first;
    first.connectToServer("test");
    QVERIFY(first.waitForConnected());
    QJsonRpcServiceSocket firstClient(&first, this);
    QSignalSpy firstSpyMessageReceived(&firstClient,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));

    // send notification
    QJsonRpcMessage notification = QJsonRpcMessage::createNotification("testNotification");
    connect(&firstClient, SIGNAL(messageReceived(QJsonRpcMessage)), &firstLoop, SLOT(quit()));
    ServiceProviderNotificationHelper helper(notification, &serviceProvider);
    QTimer::singleShot(100, &helper, SLOT(activate()));
    firstLoop.exec();

    QVERIFY(!firstSpyMessageReceived.isEmpty());

    QVariant firstMessage = firstSpyMessageReceived.takeFirst().at(0);
    QJsonRpcMessage firstNotification = firstMessage.value<QJsonRpcMessage>();
    QCOMPARE(firstNotification, notification);
}




class TestNumberParamsService : public QJsonRpcService
{
    Q_OBJECT
public:
    TestNumberParamsService(QObject *parent = 0)
        : QJsonRpcService(parent), m_called(0) {}

    QString serviceName() const
    {
        return QString("service");
    }

    int callCount() const
    {
        return m_called;
    }

public Q_SLOTS:
    void numberParameters(int intParam, double doubleParam)
    {
        if (intParam == 10 && doubleParam == 3.14159)
            m_called++;
    }

private:
    int m_called;

};

void TestQJsonRpcServiceProvider::testLocalNumberParameters()
{
    // Initialize the service provider.
    QEventLoop loop;
    TestNumberParamsService service;
    QLocalServer localServer;
    QVERIFY(localServer.listen("test"));
    QJsonRpcServiceProvider serviceProvider(&localServer);
    serviceProvider.addService(&service);

    // Connect to the socket.
    QLocalSocket socket;
    socket.connectToServer("test");
    QVERIFY(socket.waitForConnected());
    QJsonRpcServiceSocket serviceSocket(&socket, this);
    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.numberParameters", QVariantList() << 10 << 3.14159);
    QJsonRpcServiceReply *reply = serviceSocket.sendMessage(request);
    connect(&serviceSocket, SIGNAL(messageReceived(QJsonRpcMessage)), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QCOMPARE(service.callCount(), 1);
}

















void TestQJsonRpcServiceProvider::testTcpNoParameter()
{
    // Initialize the service provider.
    QEventLoop loop;
    TestService service;
    QTcpServer tcpServer;
    QVERIFY(tcpServer.listen(QHostAddress::LocalHost, 5555));
    QJsonRpcServiceProvider serviceProvider(&tcpServer);
    serviceProvider.addService(&service);

    // Connect to the socket.
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, 5555);
    QVERIFY(socket.waitForConnected());

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

void TestQJsonRpcServiceProvider::testTcpSingleParameter()
{
    // Initialize the service provider.
    QEventLoop loop;
    TestService service;
    QTcpServer tcpServer;
    QVERIFY(tcpServer.listen(QHostAddress::LocalHost, 5555));
    QJsonRpcServiceProvider serviceProvider(&tcpServer);
    serviceProvider.addService(&service);

    // Connect to the socket.
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, 5555);
    QVERIFY(socket.waitForConnected());
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

void TestQJsonRpcServiceProvider::testTcpMultiparameter()
{
    // Initialize the service provider.
    QEventLoop loop;
    TestService service;
    QTcpServer tcpServer;
    QVERIFY(tcpServer.listen(QHostAddress::LocalHost, 5555));
    QJsonRpcServiceProvider serviceProvider(&tcpServer);
    serviceProvider.addService(&service);

    // Connect to the socket.
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, 5555);
    QVERIFY(socket.waitForConnected());
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

void TestQJsonRpcServiceProvider::testTcpInvalidArgs()
{
    // Initialize the service provider.
    QEventLoop loop;
    TestService service;
    QTcpServer tcpServer;
    QVERIFY(tcpServer.listen(QHostAddress::LocalHost, 5555));
    QJsonRpcServiceProvider serviceProvider(&tcpServer);
    serviceProvider.addService(&service);

    // Connect to the socket.
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, 5555);
    QVERIFY(socket.waitForConnected());
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

void TestQJsonRpcServiceProvider::testTcpMethodNotFound()
{
    // Initialize the service provider.
    QEventLoop loop;
    TestService service;
    QTcpServer tcpServer;
    QVERIFY(tcpServer.listen(QHostAddress::LocalHost, 5555));
    QJsonRpcServiceProvider serviceProvider(&tcpServer);
    serviceProvider.addService(&service);

    // Connect to the socket.
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, 5555);
    QVERIFY(socket.waitForConnected());
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

void TestQJsonRpcServiceProvider::testTcpNotifyConnectedClients()
{
    // Initialize the service provider.
    QEventLoop firstLoop;
    TestService service;
    QTcpServer tcpServer;
    QVERIFY(tcpServer.listen(QHostAddress::LocalHost, 5555));
    QJsonRpcServiceProvider serviceProvider(&tcpServer);
    serviceProvider.addService(&service);

    // first client
    QTcpSocket first;
    first.connectToHost(QHostAddress::LocalHost, 5555);
    QVERIFY(first.waitForConnected());
    QJsonRpcServiceSocket firstClient(&first, this);
    QSignalSpy firstSpyMessageReceived(&firstClient,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));

    // send notification
    QJsonRpcMessage notification = QJsonRpcMessage::createNotification("testNotification");
    connect(&firstClient, SIGNAL(messageReceived(QJsonRpcMessage)), &firstLoop, SLOT(quit()));
    ServiceProviderNotificationHelper helper(notification, &serviceProvider);
    QTimer::singleShot(100, &helper, SLOT(activate()));
    firstLoop.exec();

    QVERIFY(!firstSpyMessageReceived.isEmpty());

    QVariant firstMessage = firstSpyMessageReceived.takeFirst().at(0);
    QJsonRpcMessage firstNotification = firstMessage.value<QJsonRpcMessage>();
    QCOMPARE(firstNotification, notification);
}


QTEST_MAIN(TestQJsonRpcServiceProvider)
#include "tst_qjsonrpcserviceprovider.moc"
