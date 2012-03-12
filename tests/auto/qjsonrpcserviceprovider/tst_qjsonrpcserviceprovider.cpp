#include <QLocalServer>
#include <QLocalSocket>

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

void TestQJsonRpcServiceProvider::testServiceProviderNoParameter()
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

void TestQJsonRpcServiceProvider::testServiceProviderSingleParameter()
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

void TestQJsonRpcServiceProvider::testServiceProviderMultiparameter()
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

void TestQJsonRpcServiceProvider::testServiceProviderInvalidArgs()
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

void TestQJsonRpcServiceProvider::testServiceProviderMethodNotFound()
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

QTEST_MAIN(TestQJsonRpcServiceProvider)
#include "tst_qjsonrpcserviceprovider.moc"
