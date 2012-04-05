#include <QLocalSocket>
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
    void testLocalVariantParameter();
    void testLocalInvalidArgs();
    void testLocalMethodNotFound();
    void testLocalInvalidRequest();
    void testLocalNotifyConnectedClients();
    void testLocalNumberParameters();
    void testLocalHugeResponse();
    void testLocalComplexMethod();
    void testLocalDefaultParameters();

    // TCP Server
    void testTcpNoParameter();
    void testTcpSingleParameter();
    void testTcpMultiparameter();
    void testTcpVariantParameter();
    void testTcpInvalidArgs();
    void testTcpMethodNotFound();
    void testTcpInvalidRequest();
    void testTcpNotifyConnectedClients();
    void testTcpNumberParameters();
    void testTcpHugeResponse();

};

class TestService : public QJsonRpcService
{
    Q_OBJECT
    Q_CLASSINFO("serviceName", "service")
public:
    TestService(QObject *parent = 0)
        : QJsonRpcService(parent) {}

public Q_SLOTS:
    void noParam() const {}
    QString singleParam(const QString &string) const { return string; }
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

    bool variantParameter(const QVariant &variantParam) const
    {
        return variantParam.toBool();
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
    QJsonRpcLocalServiceProvider serviceProvider;
    serviceProvider.addService(new TestService);
    QVERIFY(serviceProvider.listen("test"));

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
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QCOMPARE(request.id(), response.id());
}

void TestQJsonRpcServiceProvider::testLocalSingleParameter()
{
    // Initialize the service provider.
    QEventLoop loop;
    QJsonRpcLocalServiceProvider serviceProvider;
    serviceProvider.addService(new TestService);
    QVERIFY(serviceProvider.listen("test"));

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
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QCOMPARE(request.id(), response.id());
    QCOMPARE(response.result().toString(), QLatin1String("single"));
}

void TestQJsonRpcServiceProvider::testLocalMultiparameter()
{
    // Initialize the service provider.
    QEventLoop loop;
    QJsonRpcLocalServiceProvider serviceProvider;
    serviceProvider.addService(new TestService);
    QVERIFY(serviceProvider.listen("test"));

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
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QCOMPARE(request.id(), response.id());
    QCOMPARE(response.result().toString(), QLatin1String("abc"));
}

void TestQJsonRpcServiceProvider::testLocalVariantParameter()
{
    // Initialize the service provider.
    QEventLoop loop;
    QJsonRpcLocalServiceProvider serviceProvider;
    serviceProvider.addService(new TestService);
    QVERIFY(serviceProvider.listen("test"));

    // Connect to the socket.
    QLocalSocket socket;
    socket.connectToServer("test");
    QVERIFY(socket.waitForConnected());
    QJsonRpcServiceSocket serviceSocket(&socket, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));

    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.variantParameter",
                                                             QVariantList() << QVariant(true));
    QJsonRpcServiceReply *reply = serviceSocket.sendMessage(request);
    connect(&serviceSocket, SIGNAL(messageReceived(QJsonRpcMessage)), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QJsonRpcMessage response = reply->response();
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QCOMPARE(request.id(), response.id());
    QVERIFY(response.result() == true);
}

void TestQJsonRpcServiceProvider::testLocalInvalidArgs()
{
    // Initialize the service provider.
    QEventLoop loop;
    QJsonRpcLocalServiceProvider serviceProvider;
    serviceProvider.addService(new TestService);
    QVERIFY(serviceProvider.listen("test"));

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
    QVariant message = spyMessageReceived.takeFirst().at(0);
    QJsonRpcMessage error = message.value<QJsonRpcMessage>();
    QCOMPARE(request.id(), error.id());
    QVERIFY(error.errorCode() == QJsonRpc::InvalidParams);
}

void TestQJsonRpcServiceProvider::testLocalMethodNotFound()
{
    // Initialize the service provider.
    QEventLoop loop;
    QJsonRpcLocalServiceProvider serviceProvider;
    serviceProvider.addService(new TestService);
    QVERIFY(serviceProvider.listen("test"));

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
    QVERIFY(reply->response().isValid());
    QVariant message = spyMessageReceived.takeFirst().at(0);
    QJsonRpcMessage error = message.value<QJsonRpcMessage>();
    QCOMPARE(request.id(), error.id());
    QVERIFY(error.errorCode() == QJsonRpc::MethodNotFound);
}

void TestQJsonRpcServiceProvider::testLocalInvalidRequest()
{
    // Initialize the service provider.
    QEventLoop loop;
    QJsonRpcLocalServiceProvider serviceProvider;
    serviceProvider.addService(new TestService);
    QVERIFY(serviceProvider.listen("test"));

    // Connect to the socket.
    QLocalSocket socket;
    socket.connectToServer("test");
    QVERIFY(socket.waitForConnected());
    QJsonRpcServiceSocket serviceSocket(&socket, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));

    const char *invalid = "{\"jsonrpc\": \"2.0\", \"id\": 666}";

    QJsonDocument doc = QJsonDocument::fromJson(invalid);
    QJsonRpcMessage request(doc.object());
    QJsonRpcServiceReply *reply = serviceSocket.sendMessage(request);
    connect(&serviceSocket, SIGNAL(messageReceived(QJsonRpcMessage)), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QCOMPARE(spyMessageReceived.count(), 1);
    QVariant message = spyMessageReceived.takeFirst().at(0);
    QJsonRpcMessage error = message.value<QJsonRpcMessage>();
    QCOMPARE(request.id(), error.id());
    QVERIFY(error.errorCode() == QJsonRpc::InvalidRequest);
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
    QJsonRpcLocalServiceProvider serviceProvider;
    QVERIFY(serviceProvider.listen("test"));
    serviceProvider.addService(new TestService);

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

    QCOMPARE(firstSpyMessageReceived.count(), 1);
    QVariant firstMessage = firstSpyMessageReceived.takeFirst().at(0);
    QJsonRpcMessage firstNotification = firstMessage.value<QJsonRpcMessage>();
    QCOMPARE(firstNotification, notification);
}


class TestNumberParamsService : public QJsonRpcService
{
    Q_OBJECT
    Q_CLASSINFO("serviceName", "service")
public:
    TestNumberParamsService(QObject *parent = 0)
        : QJsonRpcService(parent), m_called(0) {}

    int callCount() const { return m_called; }

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
    TestNumberParamsService *service = new TestNumberParamsService;
    QJsonRpcLocalServiceProvider serviceProvider;
    serviceProvider.addService(service);
    QVERIFY(serviceProvider.listen("test"));

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

    QCOMPARE(service->callCount(), 1);
}

class TestHugeResponseService : public QJsonRpcService
{
    Q_OBJECT
    Q_CLASSINFO("serviceName", "service")
public:
    TestHugeResponseService(QObject *parent = 0)
        : QJsonRpcService(parent) {}

public Q_SLOTS:
    QVariantMap hugeResponse()
    {
        QVariantMap result;
        for (int i = 0; i < 1000; i++) {
            QString key = QString("testKeyForHugeResponse%1").arg(i);
            result[key] = "some sample data to make the response larger";
        }
        return result;
    }
};

void TestQJsonRpcServiceProvider::testLocalHugeResponse()
{
    // Initialize the service provider.
    QEventLoop loop;
    QJsonRpcLocalServiceProvider serviceProvider;
    serviceProvider.addService(new TestHugeResponseService);
    QVERIFY(serviceProvider.listen("test"));

    // Connect to the socket.
    QLocalSocket socket;
    socket.connectToServer("test");
    QVERIFY(socket.waitForConnected());
    QJsonRpcServiceSocket serviceSocket(&socket, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));

    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.hugeResponse");
    QJsonRpcServiceReply *reply = serviceSocket.sendMessage(request);
    connect(&serviceSocket, SIGNAL(messageReceived(QJsonRpcMessage)), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(reply->response().isValid());
}

class TestComplexMethodService : public QJsonRpcService
{
    Q_OBJECT
    Q_CLASSINFO("serviceName", "service.complex.prefix.for")
public:
    TestComplexMethodService(QObject *parent = 0)
        : QJsonRpcService(parent) {}

public Q_SLOTS:
    void testMethod() {}
};

void TestQJsonRpcServiceProvider::testLocalComplexMethod()
{
    // Initialize the service provider.
    QEventLoop loop;
    QJsonRpcLocalServiceProvider serviceProvider;
    serviceProvider.addService(new TestComplexMethodService);
    QVERIFY(serviceProvider.listen("test"));

    // Connect to the socket.
    QLocalSocket socket;
    socket.connectToServer("test");
    QVERIFY(socket.waitForConnected());
    QJsonRpcServiceSocket serviceSocket(&socket, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                  SIGNAL(messageReceived(QJsonRpcMessage)));

    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.complex.prefix.for.testMethod");
    QJsonRpcServiceReply *reply = serviceSocket.sendMessage(request);
    connect(&serviceSocket, SIGNAL(messageReceived(QJsonRpcMessage)), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QJsonRpcMessage response = reply->response();
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QCOMPARE(request.id(), response.id());
}

class TestDefaultParametersService : public QJsonRpcService
{
    Q_OBJECT
    Q_CLASSINFO("serviceName", "service")
public:
    TestDefaultParametersService(QObject *parent = 0)
        : QJsonRpcService(parent) {}

public Q_SLOTS:
    QString testMethod(const QString &name = QString()) {
        if (name.isEmpty())
            return "empty string";
        return QString("hello %1").arg(name);
    }

    QString testMethod2(const QString &name = QString(), int year = 2012)
    {
        return QString("%1%2").arg(name).arg(year);
    }
};

void TestQJsonRpcServiceProvider::testLocalDefaultParameters()
{
    // Initialize the service provider.
    QEventLoop loop;
    QJsonRpcLocalServiceProvider serviceProvider;
    serviceProvider.addService(new TestDefaultParametersService);
    QVERIFY(serviceProvider.listen("test"));

    // Connect to the socket.
    QLocalSocket socket;
    socket.connectToServer("test");
    QVERIFY(socket.waitForConnected());
    QJsonRpcServiceSocket serviceSocket(&socket, this);

    // test without name
    QJsonRpcMessage noNameRequest = QJsonRpcMessage::createRequest("service.testMethod");
    QJsonRpcServiceReply *noNameReply = serviceSocket.sendMessage(noNameRequest);
    connect(noNameReply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    QCOMPARE(noNameReply->response().result().toString(), QLatin1String("empty string"));

    // test with name
    QJsonRpcMessage nameRequest = QJsonRpcMessage::createRequest("service.testMethod", QLatin1String("matt"));
    QJsonRpcServiceReply *nameReply = serviceSocket.sendMessage(nameRequest);
    connect(nameReply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    QCOMPARE(nameReply->response().result().toString(), QLatin1String("hello matt"));

    // test multiparameter
    QJsonRpcMessage konyRequest = QJsonRpcMessage::createRequest("service.testMethod2", QLatin1String("KONY"));
    QJsonRpcServiceReply *konyReply = serviceSocket.sendMessage(konyRequest);
    connect(konyReply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    QCOMPARE(konyReply->response().result().toString(), QLatin1String("KONY2012"));
}















void TestQJsonRpcServiceProvider::testTcpNoParameter()
{
    // Initialize the service provider.
    QEventLoop loop;
    TestService service;
    QJsonRpcTcpServiceProvider serviceProvider;
    serviceProvider.addService(new TestService);
    QVERIFY(serviceProvider.listen(QHostAddress::LocalHost, 5555));

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
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QCOMPARE(request.id(), response.id());
}

void TestQJsonRpcServiceProvider::testTcpSingleParameter()
{
    // Initialize the service provider.
    QEventLoop loop;
    QJsonRpcTcpServiceProvider serviceProvider;
    serviceProvider.addService(new TestService);
    QVERIFY(serviceProvider.listen(QHostAddress::LocalHost, 5555));

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
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QCOMPARE(request.id(), response.id());
    QVERIFY(response.result() == QString("single"));
}

void TestQJsonRpcServiceProvider::testTcpMultiparameter()
{
    // Initialize the service provider.
    QEventLoop loop;
    QJsonRpcTcpServiceProvider serviceProvider;
    serviceProvider.addService(new TestService);
    QVERIFY(serviceProvider.listen(QHostAddress::LocalHost, 5555));

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
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QCOMPARE(request.id(), response.id());
    QVERIFY(response.result() == QString("abc"));
}

void TestQJsonRpcServiceProvider::testTcpVariantParameter()
{
    // Initialize the service provider.
    QEventLoop loop;
    QJsonRpcTcpServiceProvider serviceProvider;
    serviceProvider.addService(new TestService);
    QVERIFY(serviceProvider.listen(QHostAddress::LocalHost, 5555));

    // Connect to the socket.
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, 5555);
    QVERIFY(socket.waitForConnected());
    QJsonRpcServiceSocket serviceSocket(&socket, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));

    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.variantParameter",
                                                             QVariantList() << QVariant(true));
    QJsonRpcServiceReply *reply = serviceSocket.sendMessage(request);
    connect(&serviceSocket, SIGNAL(messageReceived(QJsonRpcMessage)), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QJsonRpcMessage response = reply->response();
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QCOMPARE(request.id(), response.id());
    QVERIFY(response.result() == true);
}

void TestQJsonRpcServiceProvider::testTcpInvalidArgs()
{
    // Initialize the service provider.
    QEventLoop loop;
    QJsonRpcTcpServiceProvider serviceProvider;
    serviceProvider.addService(new TestService);
    QVERIFY(serviceProvider.listen(QHostAddress::LocalHost, 5555));

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
    QVariant message = spyMessageReceived.takeFirst().at(0);
    QJsonRpcMessage error = message.value<QJsonRpcMessage>();
    QCOMPARE(request.id(), error.id());
    QVERIFY(error.errorCode() == QJsonRpc::InvalidParams);
}

void TestQJsonRpcServiceProvider::testTcpMethodNotFound()
{
    // Initialize the service provider.
    QEventLoop loop;
    QJsonRpcTcpServiceProvider serviceProvider;
    serviceProvider.addService(new TestService);
    QVERIFY(serviceProvider.listen(QHostAddress::LocalHost, 5555));

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
    QVariant message = spyMessageReceived.takeFirst().at(0);
    QJsonRpcMessage error = message.value<QJsonRpcMessage>();
    QCOMPARE(request.id(), error.id());
    QVERIFY(error.errorCode() == QJsonRpc::MethodNotFound);
}

void TestQJsonRpcServiceProvider::testTcpInvalidRequest()
{
    // Initialize the service provider.
    QEventLoop loop;
    QJsonRpcTcpServiceProvider serviceProvider;
    serviceProvider.addService(new TestService);
    QVERIFY(serviceProvider.listen(QHostAddress::LocalHost, 5555));

    // Connect to the socket.
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, 5555);
    QVERIFY(socket.waitForConnected());
    QJsonRpcServiceSocket serviceSocket(&socket, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));

    const char *invalid = "{\"jsonrpc\": \"2.0\", \"id\": 666}";

    QJsonDocument doc = QJsonDocument::fromJson(invalid);
    QJsonRpcMessage request(doc.object());
    QJsonRpcServiceReply *reply = serviceSocket.sendMessage(request);
    connect(&serviceSocket, SIGNAL(messageReceived(QJsonRpcMessage)), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QCOMPARE(spyMessageReceived.count(), 1);
    QVariant message = spyMessageReceived.takeFirst().at(0);
    QJsonRpcMessage error = message.value<QJsonRpcMessage>();
    QCOMPARE(request.id(), error.id());
    QVERIFY(error.errorCode() == QJsonRpc::InvalidRequest);
}

void TestQJsonRpcServiceProvider::testTcpNotifyConnectedClients()
{
    // Initialize the service provider.
    QEventLoop firstLoop;
    QJsonRpcTcpServiceProvider serviceProvider;
    serviceProvider.addService(new TestService);
    QVERIFY(serviceProvider.listen(QHostAddress::LocalHost, 5555));

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

    QCOMPARE(firstSpyMessageReceived.count(), 1);
    QVariant firstMessage = firstSpyMessageReceived.takeFirst().at(0);
    QJsonRpcMessage firstNotification = firstMessage.value<QJsonRpcMessage>();
    QCOMPARE(firstNotification, notification);
}

void TestQJsonRpcServiceProvider::testTcpNumberParameters()
{
    // Initialize the service provider.
    QEventLoop loop;
    TestNumberParamsService *service = new TestNumberParamsService;
    QJsonRpcTcpServiceProvider serviceProvider;
    serviceProvider.addService(service);
    QVERIFY(serviceProvider.listen(QHostAddress::LocalHost, 5555));

    // Connect to the socket.
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, 5555);
    QVERIFY(socket.waitForConnected());
    QJsonRpcServiceSocket serviceSocket(&socket, this);
    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.numberParameters", QVariantList() << 10 << 3.14159);
    QJsonRpcServiceReply *reply = serviceSocket.sendMessage(request);
    connect(&serviceSocket, SIGNAL(messageReceived(QJsonRpcMessage)), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QCOMPARE(service->callCount(), 1);
}

void TestQJsonRpcServiceProvider::testTcpHugeResponse()
{
    // Initialize the service provider.
    QEventLoop loop;
    TestHugeResponseService service;
    QJsonRpcTcpServiceProvider serviceProvider;
    serviceProvider.addService(new TestHugeResponseService);
    QVERIFY(serviceProvider.listen(QHostAddress::LocalHost, 5555));

    // Connect to the socket.
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, 5555);
    QVERIFY(socket.waitForConnected());
    QJsonRpcServiceSocket serviceSocket(&socket, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));

    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.hugeResponse");
    QJsonRpcServiceReply *reply = serviceSocket.sendMessage(request);
    connect(&serviceSocket, SIGNAL(messageReceived(QJsonRpcMessage)), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QCOMPARE(spyMessageReceived.count(), 1);
}

QTEST_MAIN(TestQJsonRpcServiceProvider)
#include "tst_qjsonrpcserviceprovider.moc"
