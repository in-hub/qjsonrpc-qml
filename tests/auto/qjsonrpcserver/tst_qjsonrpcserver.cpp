/*
 * Copyright (C) 2012-2013 Matt Broadstone
 * Contact: http://bitbucket.org/devonit/qjsonrpc
 *
 * This file is part of the QJsonRpc Library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */
#include <QLocalSocket>
#include <QTcpSocket>
#include <QScopedPointer>

#include <QtCore/QEventLoop>
#include <QtCore/QVariant>
#include <QtTest/QtTest>

#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#else
#include "json/qjsondocument.h"
#endif

#include "qjsonrpcabstractserver.h"
#include "qjsonrpclocalserver.h"
#include "qjsonrpctcpserver.h"
#include "qjsonrpcsocket.h"
#include "qjsonrpcmessage.h"
#include "qjsonrpcservicereply.h"
#include "testservices.h"

bool waitForSignal(QObject *obj, const char *signal, int delay = 5)
{
    QObject::connect(obj, signal, &QTestEventLoop::instance(), SLOT(exitLoop()));
    QPointer<QObject> safe = obj;

    QTestEventLoop::instance().enterLoop(delay);
    if (!safe.isNull())
        QObject::disconnect(safe, signal, &QTestEventLoop::instance(), SLOT(exitLoop()));
    return !QTestEventLoop::instance().timeout();
}

class TestQJsonRpcServer: public QObject
{
    Q_OBJECT
public:
    TestQJsonRpcServer();

    enum ServerType {
        TcpServer,
        LocalServer
    };

private Q_SLOTS:
    void initTestCase_data();
    void init();
    void cleanup();

    void noParameter();
    void singleParameter();
    void multiParameter();
    void variantParameter();
    void variantListParameter();
    void variantResult();
    void invalidArgs();
    void methodNotFound();
    void invalidRequest();
    void notifyConnectedClients_data();
    void notifyConnectedClients();
    void numberParameters();
    void hugeResponse();
    void complexMethod();
    void defaultParameters();
    void overloadedMethod();
    void qVariantMapInvalidParam();
    void stringListParameter();
    void outputParameter();
    void userDeletedReplyOnDelayedResponse();

    void addRemoveService();
    void serviceWithNoGivenName();
    void cantRemoveInvalidService();
    void cantAddServiceTwice();

private:
    QScopedPointer<QJsonRpcAbstractServer> server;
    QScopedPointer<QJsonRpcSocket> clientSocket;
    QScopedPointer<QJsonRpcSocket> serverSocket;
    QPointer<QTcpSocket> tcpSocket;
    QPointer<QLocalSocket> localSocket;

private:
    // fix later
    // void testListOfInts();

};
Q_DECLARE_METATYPE(TestQJsonRpcServer::ServerType)
Q_DECLARE_METATYPE(QJsonRpcMessage::Type)

#if QT_VERSION < 0x050000
Q_DECLARE_METATYPE(QJsonArray)
#endif

TestQJsonRpcServer::TestQJsonRpcServer()
    : server(0),
      clientSocket(0),
      serverSocket(0)
{
}

void TestQJsonRpcServer::initTestCase_data()
{
    QTest::addColumn<ServerType>("serverType");
    QTest::newRow("tcp") << TcpServer;
    QTest::newRow("local") << LocalServer;
}

void TestQJsonRpcServer::init()
{
    QFETCH_GLOBAL(ServerType, serverType);
    if (serverType == LocalServer) {
        QJsonRpcLocalServer *s = new QJsonRpcLocalServer(this);
        QVERIFY(s->listen("qjsonrpc-test-local-server"));
        server.reset(s);

        localSocket = new QLocalSocket(this);
        localSocket->connectToServer("qjsonrpc-test-local-server");
        QVERIFY(localSocket->waitForConnected());
        clientSocket.reset(new QJsonRpcSocket(localSocket, this));
    } else if (serverType == TcpServer) {
        QJsonRpcTcpServer *s = new QJsonRpcTcpServer(this);
        QVERIFY(s->listen(QHostAddress::LocalHost, quint16(91919)));
        server.reset(s);

        tcpSocket = new QTcpSocket(this);
        tcpSocket->connectToHost(QHostAddress::LocalHost, quint16(91919));
        QVERIFY(tcpSocket->waitForConnected());
        clientSocket.reset(new QJsonRpcSocket(tcpSocket, this));
    }

    QVERIFY(waitForSignal(server.data(), SIGNAL(clientConnected())));
    QCOMPARE(server->connectedClientCount(), 1);
}

void TestQJsonRpcServer::cleanup()
{
    QFETCH_GLOBAL(ServerType, serverType);
    if (serverType == TcpServer || serverType == LocalServer) {
        if (!tcpSocket.isNull() && tcpSocket->state() == QAbstractSocket::ConnectedState) {
            tcpSocket->disconnectFromHost();
            QVERIFY(waitForSignal(server.data(), SIGNAL(clientDisconnected())));
        }

        if (!localSocket.isNull() && localSocket->state() == QLocalSocket::ConnectedState) {
            localSocket->disconnectFromServer();
            QVERIFY(waitForSignal(server.data(), SIGNAL(clientDisconnected())));
        }

        QCOMPARE(server->connectedClientCount(), 0);
    }

    server->close();
}

void TestQJsonRpcServer::noParameter()
{
    QVERIFY(server->addService(new TestService));

    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));
    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.noParam");
    QJsonRpcMessage response = clientSocket->sendMessageBlocking(request);
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QCOMPARE(request.id(), response.id());
    QCOMPARE(spyMessageReceived.count(), 1);
}

void TestQJsonRpcServer::singleParameter()
{
    QVERIFY(server->addService(new TestService));

    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));
    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.singleParam", QString("single"));
    QJsonRpcMessage response = clientSocket->sendMessageBlocking(request);
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QCOMPARE(request.id(), response.id());
    QCOMPARE(response.result().toString(), QLatin1String("single"));
}

void TestQJsonRpcServer::overloadedMethod()
{
    QVERIFY(server->addService(new TestService));
    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));

    QJsonRpcMessage stringRequest = QJsonRpcMessage::createRequest("service.overloadedMethod", QString("single"));
    QJsonRpcMessage stringResponse = clientSocket->sendMessageBlocking(stringRequest);
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(stringResponse.errorCode() == QJsonRpc::NoError);
    QCOMPARE(stringRequest.id(), stringResponse.id());
    QCOMPARE(stringResponse.result().toBool(), false);

    QJsonRpcMessage intRequest = QJsonRpcMessage::createRequest("service.overloadedMethod", 10);
    QJsonRpcMessage intResponse = clientSocket->sendMessageBlocking(intRequest);
    QCOMPARE(spyMessageReceived.count(), 2);
    QVERIFY(intResponse.errorCode() == QJsonRpc::NoError);
    QCOMPARE(intRequest.id(), intResponse.id());
    QCOMPARE(intResponse.result().toBool(), true);

    QVariantMap testMap;
    testMap["one"] = 1;
    testMap["two"] = 2;
    testMap["three"] = 3;
    QJsonRpcMessage mapRequest =
        QJsonRpcMessage::createRequest("service.overloadedMethod", QJsonValue::fromVariant(testMap));
    QJsonRpcMessage mapResponse = clientSocket->sendMessageBlocking(mapRequest);
    QCOMPARE(spyMessageReceived.count(), 3);
    QVERIFY(mapResponse.errorCode() == QJsonRpc::InvalidParams);
    QCOMPARE(mapRequest.id(), mapResponse.id());
}

void TestQJsonRpcServer::multiParameter()
{
    QVERIFY(server->addService(new TestService));

    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));

    QJsonArray params;
    params.append(QLatin1String("a"));
    params.append(QLatin1String("b"));
    params.append(QLatin1String("c"));
    QJsonRpcMessage request =
        QJsonRpcMessage::createRequest("service.multipleParam", params);
    QJsonRpcMessage response = clientSocket->sendMessageBlocking(request);
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QCOMPARE(request.id(), response.id());
    QCOMPARE(response.result().toString(), QLatin1String("abc"));
}

void TestQJsonRpcServer::variantParameter()
{
    QVERIFY(server->addService(new TestService));

    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));

    QJsonArray params;
    params.append(QJsonValue::fromVariant(QVariant(true)));
    QJsonRpcMessage request =
        QJsonRpcMessage::createRequest("service.variantParameter", params);
    QJsonRpcMessage response = clientSocket->sendMessageBlocking(request);
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QCOMPARE(request.id(), response.id());
    QVERIFY(response.result() == true);
}

void TestQJsonRpcServer::variantListParameter()
{
    QVERIFY(server->addService(new TestService));

    QJsonArray data;
    data.append(1);
    data.append(20);
    data.append(QLatin1String("hello"));
    data.append(false);

    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));

    QJsonArray params;
    params.append(data);
    QJsonRpcMessage request =
        QJsonRpcMessage::createRequest("service.variantListParameter", params);
    QJsonRpcMessage response = clientSocket->sendMessageBlocking(request);
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QCOMPARE(request.id(), response.id());
    QCOMPARE(response.result().toArray(), data);
}

void TestQJsonRpcServer::variantResult()
{
    QVERIFY(server->addService(new TestService));

    QJsonRpcMessage response =
        clientSocket->invokeRemoteMethodBlocking("service.variantStringResult");
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QString stringResult = response.result().toString();
    QCOMPARE(stringResult, QLatin1String("hello"));
}

void TestQJsonRpcServer::invalidArgs()
{
    QVERIFY(server->addService(new TestService));

    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));
    QJsonRpcMessage request =
        QJsonRpcMessage::createRequest("service.noParam", false);
    clientSocket->sendMessageBlocking(request);
    QCOMPARE(spyMessageReceived.count(), 1);
    QVariant message = spyMessageReceived.takeFirst().at(0);
    QJsonRpcMessage error = message.value<QJsonRpcMessage>();
    QCOMPARE(request.id(), error.id());
    QVERIFY(error.errorCode() == QJsonRpc::InvalidParams);
}

void TestQJsonRpcServer::methodNotFound()
{
    QVERIFY(server->addService(new TestService));

    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));
    QJsonRpcMessage request =
        QJsonRpcMessage::createRequest("service.doesNotExist");
    QJsonRpcMessage response = clientSocket->sendMessageBlocking(request);
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(response.isValid());
    QVariant message = spyMessageReceived.takeFirst().at(0);
    QJsonRpcMessage error = message.value<QJsonRpcMessage>();
    QCOMPARE(request.id(), error.id());
    QVERIFY(error.errorCode() == QJsonRpc::MethodNotFound);
}

void TestQJsonRpcServer::invalidRequest()
{
    QVERIFY(server->addService(new TestService));

    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));
    QJsonRpcMessage request = QJsonRpcMessage::fromJson("{\"jsonrpc\": \"2.0\", \"id\": 666}");
    clientSocket->sendMessageBlocking(request);

    QCOMPARE(spyMessageReceived.count(), 1);
    QVariant message = spyMessageReceived.takeFirst().at(0);
    QJsonRpcMessage error = message.value<QJsonRpcMessage>();
    QCOMPARE(request.id(), error.id());
    QVERIFY(error.errorCode() == QJsonRpc::InvalidRequest);
}

void TestQJsonRpcServer::qVariantMapInvalidParam()
{
    QVERIFY(server->addService(new TestService));

    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));
    const char *invalid = "{\"jsonrpc\": \"2.0\", \"id\": 0, \"method\": \"service.variantMapInvalidParam\",\"params\": [[{\"foo\":\"bar\",\"baz\":\"quux\"}, {\"foo\":\"bar\"}]]}";
    QJsonRpcMessage request = QJsonRpcMessage::fromJson(invalid);
    clientSocket->sendMessageBlocking(request);

    QCOMPARE(spyMessageReceived.count(), 1);
    QVariant message = spyMessageReceived.takeFirst().at(0);
    QJsonRpcMessage error = message.value<QJsonRpcMessage>();
    QCOMPARE(request.id(), error.id());
    QVERIFY(error.errorCode() == QJsonRpc::InvalidParams);
}

class ServerNotificationHelper : public QObject
{
    Q_OBJECT
public:
    ServerNotificationHelper(const QJsonRpcMessage &message, QJsonRpcAbstractServer *provider)
        : m_provider(provider),
          m_notification(message) {}

public Q_SLOTS:
    void activate() {
        m_provider->notifyConnectedClients(m_notification);
    }

private:
    QJsonRpcAbstractServer *m_provider;
    QJsonRpcMessage m_notification;

};

void TestQJsonRpcServer::notifyConnectedClients_data()
{
    QTest::addColumn<QString>("method");
    QTest::addColumn<QJsonRpcMessage::Type>("type");
    QTest::addColumn<QJsonArray>("parameters");
    QTest::addColumn<bool>("sendAsMessage");

    QTest::newRow("notification-message") << "testNotification" << QJsonRpcMessage::Notification << QJsonArray() << true;
    QTest::newRow("notification-direct") << "testNotification" << QJsonRpcMessage::Notification << QJsonArray() << false;

    QJsonArray parameters;
    parameters.append(QLatin1String("test"));
    QTest::newRow("request-message") << "testRequest" << QJsonRpcMessage::Request << parameters << true;
    QTest::newRow("request-direct") << "testRequest" << QJsonRpcMessage::Request << parameters << false;
}

void TestQJsonRpcServer::notifyConnectedClients()
{
    QFETCH(QString, method);
    QFETCH(QJsonRpcMessage::Type, type);
    QFETCH(QJsonArray, parameters);
    QFETCH(bool, sendAsMessage);

    QVERIFY(server->addService(new TestService));

    QEventLoop loop;
    connect(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)),
            &loop, SLOT(quit()));

    QSignalSpy spy(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));
    QJsonRpcMessage message;
    if (sendAsMessage) {
        switch (type) {
        case QJsonRpcMessage::Request:
            message = QJsonRpcMessage::createRequest(method, parameters);
            break;
        case QJsonRpcMessage::Notification:
            message = QJsonRpcMessage::createNotification(method, parameters);
            break;
        default:
            break;
        }
        server->notifyConnectedClients(message);
    } else {
        server->notifyConnectedClients(method, parameters);
    }
    QTimer::singleShot(2000, &loop, SLOT(quit()));
    loop.exec();

    QCOMPARE(spy.count(), 1);
    QJsonRpcMessage receivedMessage = spy.takeFirst().first().value<QJsonRpcMessage>();
    if (sendAsMessage) {
        QCOMPARE(receivedMessage, message);
    } else {
        QCOMPARE(receivedMessage.method(), method);
        QCOMPARE(receivedMessage.params().toArray(), parameters);
    }
}

void TestQJsonRpcServer::numberParameters()
{
    TestNumberParamsService *service = new TestNumberParamsService;
    QVERIFY(server->addService(service));

    QJsonArray params;
    params.append(10);
    params.append(3.14159);
    QJsonRpcMessage request =
        QJsonRpcMessage::createRequest("service.numberParameters", params);
    clientSocket->sendMessageBlocking(request);
    QCOMPARE(service->callCount(), 1);
}

void TestQJsonRpcServer::hugeResponse()
{
    QVERIFY(server->addService(new TestHugeResponseService));
    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));
    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.hugeResponse");
    QJsonRpcMessage response = clientSocket->sendMessageBlocking(request);
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(response.isValid());
}

void TestQJsonRpcServer::complexMethod()
{
    QVERIFY(server->addService(new TestComplexMethodService));
    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));
    QJsonRpcMessage request =
        QJsonRpcMessage::createRequest("service.complex.prefix.for.testMethod");
    QJsonRpcMessage response = clientSocket->sendMessageBlocking(request);
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QCOMPARE(request.id(), response.id());
}

void TestQJsonRpcServer::defaultParameters()
{
    QVERIFY(server->addService(new TestDefaultParametersService));

    // test without name
    QJsonRpcMessage noNameRequest =
        QJsonRpcMessage::createRequest("service.testMethod");
    QJsonRpcMessage response = clientSocket->sendMessageBlocking(noNameRequest);
    QVERIFY(response.type() != QJsonRpcMessage::Error);
    QCOMPARE(response.result().toString(), QLatin1String("empty string"));

    // test with name
    QJsonRpcMessage nameRequest =
        QJsonRpcMessage::createRequest("service.testMethod", QLatin1String("matt"));
    response = clientSocket->sendMessageBlocking(nameRequest);
    QVERIFY(response.type() != QJsonRpcMessage::Error);
    QCOMPARE(response.result().toString(), QLatin1String("hello matt"));

    // test multiparameter
    QJsonRpcMessage konyRequest =
        QJsonRpcMessage::createRequest("service.testMethod2", QLatin1String("KONY"));
    response = clientSocket->sendMessageBlocking(konyRequest);
    QVERIFY(response.type() != QJsonRpcMessage::Error);
    QCOMPARE(response.result().toString(), QLatin1String("KONY2012"));
}

/*
void TestQJsonRpcServer::notifyServiceSocket()
{

    // Connect to the socket.
    QLocalSocket socket;
    socket.connectToServer("test");
    QVERIFY(socket.waitForConnected());

    QJsonRpcServiceSocket serviceSocket(&socket);
    TestNumberParamsService *service = new TestNumberParamsService;
    serviceSocket.addService(service);
    QCOMPARE(service->callCount(), 0);

    QEventLoop test;
    QTimer::singleShot(10, &test, SLOT(quit()));
    test.exec();
    serviceProvider.notifyConnectedClients("service.numberParameters", QJsonArray() << 10 << 3.14159);
    QTimer::singleShot(10, &test, SLOT(quit()));
    test.exec();

    QCOMPARE(service->callCount(), 1);
}
*/

/*
Q_DECLARE_METATYPE(QList<int>)
void TestQJsonRpcServer::testListOfInts()
{
    server->addService(new TestService);
    qRegisterMetaType<QList<int> >("QList<int>");
    QList<int> intList = QList<int>() << 300 << 30 << 3;
    QVariant variant = QVariant::fromValue(intList);
    QJsonRpcMessage intRequest =
        QJsonRpcMessage::createRequest("service.methodWithListOfInts", variant);
    QJsonRpcMessage response = clientSocket->sendMessageBlocking(intRequest);
    QVERIFY(response.type() != QJsonRpcMessage::Error);
    QVERIFY(response.result().toBool());
}
*/

void TestQJsonRpcServer::stringListParameter()
{
    QVERIFY(server->addService(new TestService));
    QStringList strings = QStringList() << "one" << "two" << "three";

    QJsonArray params;
    params.append(1);
    params.append(QLatin1String("A"));
    params.append(QLatin1String("B"));
    params.append(QJsonValue::fromVariant(strings));
    QJsonRpcMessage strRequest =
        QJsonRpcMessage::createRequest("service.stringListParameter", params);
    QJsonRpcMessage response = clientSocket->sendMessageBlocking(strRequest);
    QVERIFY(response.type() != QJsonRpcMessage::Error);
    QVERIFY(response.result().toBool());
}

void TestQJsonRpcServer::outputParameter()
{
    QVERIFY(server->addService(new TestService));

    // use argument 2 as in/out parameter
    QJsonArray arrParams;
    arrParams.push_back(1);
    arrParams.push_back(0);
    arrParams.push_back(2);
    QJsonRpcMessage strRequest =
        QJsonRpcMessage::createRequest("service.outputParameter", arrParams);
    QJsonRpcMessage response = clientSocket->sendMessageBlocking(strRequest);
    QVERIFY(response.type() != QJsonRpcMessage::Error);
    QCOMPARE((int) response.result().toDouble(), 3);

    // only input parameters are provided
    QJsonObject objParams;
    objParams["in1"] = 1;
    objParams["in2"] = 3;
    strRequest =
        QJsonRpcMessage::createRequest("service.outputParameter", objParams);
    response = clientSocket->sendMessageBlocking(strRequest);
    QVERIFY(response.type() != QJsonRpcMessage::Error);
    QCOMPARE((int) response.result().toDouble(), 4);

    // also provide the in/out parameter
    objParams["out"] = 2;
    strRequest =
        QJsonRpcMessage::createRequest("service.outputParameter", objParams);
    response = clientSocket->sendMessageBlocking(strRequest);
    QVERIFY(response.type() != QJsonRpcMessage::Error);
    QCOMPARE((int) response.result().toDouble(), 6);

    // test strings
    QJsonArray stringParams;
    stringParams.push_back(QLatin1String("Sherlock"));
    stringParams.push_back(QLatin1String(""));
    stringParams.push_back(QLatin1String("Holmes"));
    strRequest =
        QJsonRpcMessage::createRequest("service.outputParameterWithStrings", stringParams);
    response = clientSocket->sendMessageBlocking(strRequest);
    QVERIFY(response.type() != QJsonRpcMessage::Error);
    QCOMPARE(response.result().toString(), QLatin1String("Sherlock Holmes"));

    // only input parameters are provided
    QJsonObject stringObjectParams;
    stringObjectParams["first"] = QLatin1String("Sherlock");
    stringObjectParams["output"] = QLatin1String("Hello");
    stringObjectParams["last"] = QLatin1String("Holmes");
    strRequest =
        QJsonRpcMessage::createRequest("service.outputParameterWithStrings", stringObjectParams);
    response = clientSocket->sendMessageBlocking(strRequest);
    QVERIFY(response.type() != QJsonRpcMessage::Error);
    QCOMPARE(response.result().toString(), QLatin1String("Hello Sherlock Holmes"));
}

void TestQJsonRpcServer::userDeletedReplyOnDelayedResponse()
{
    QVERIFY(server->addService(new TestService));
    QJsonRpcMessage request =
        QJsonRpcMessage::createRequest("service.delayedResponse");
    QJsonRpcServiceReply *reply = clientSocket->sendMessage(request);
    delete reply;

    // this is cheesy...
    for (int i = 0; i < 10; i++)
        qApp->processEvents();
}

void TestQJsonRpcServer::addRemoveService()
{
    TestService service;
    QVERIFY(server->addService(&service));

    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));
    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.noParam");
    QJsonRpcMessage response = clientSocket->sendMessageBlocking(request);
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QCOMPARE(request.id(), response.id());
    QCOMPARE(spyMessageReceived.count(), 1);

    QVERIFY(server->removeService(&service));
    response = clientSocket->sendMessageBlocking(request);
    QVERIFY(response.errorCode() == QJsonRpc::MethodNotFound);
    QVERIFY(server->errorString().isEmpty());
}

void TestQJsonRpcServer::serviceWithNoGivenName()
{
    QVERIFY(server->addService(new TestServiceWithoutServiceName));
    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));
    QJsonRpcMessage request =
        QJsonRpcMessage::createRequest("testservicewithoutservicename.testMethod", QLatin1String("foo"));
    QJsonRpcMessage response = clientSocket->sendMessageBlocking(request);
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QCOMPARE(request.id(), response.id());
    QCOMPARE(spyMessageReceived.count(), 1);
}

void TestQJsonRpcServer::cantRemoveInvalidService()
{
    TestService service;
    QCOMPARE(server->removeService(&service), false);
}

void TestQJsonRpcServer::cantAddServiceTwice()
{
    TestService service;
    QVERIFY(server->addService(&service));
    QCOMPARE(server->addService(&service), false);
}

QTEST_MAIN(TestQJsonRpcServer)
#include "tst_qjsonrpcserver.moc"
