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

#include "qjsonrpcabstractserver_p.h"
#include "qjsonrpcabstractserver.h"
#include "qjsonrpclocalserver.h"
#include "qjsonrpctcpserver.h"
#include "qjsonrpcsocket.h"
#include "qjsonrpcservice_p.h"
#include "qjsonrpcservice.h"
#include "qjsonrpcmessage.h"
#include "qjsonrpcservicereply.h"

bool waitForSignal(QObject *obj, const char *signal, int delay = 5)
{
    QObject::connect(obj, signal, &QTestEventLoop::instance(), SLOT(exitLoop()));
    QPointer<QObject> safe = obj;

    QTestEventLoop::instance().enterLoop(delay);
    if (!safe.isNull())
        QObject::disconnect(safe, signal, &QTestEventLoop::instance(), SLOT(exitLoop()));
    return !QTestEventLoop::instance().timeout();
}

class FakeQJsonRpcSocket : public QJsonRpcSocket
{
    Q_OBJECT
public:
    FakeQJsonRpcSocket(QIODevice *device, QObject *parent = 0)
        : QJsonRpcSocket(device, parent),
          m_buffer(0)
    {
        m_buffer = new QBuffer(this);
        m_buffer->open(QIODevice::ReadWrite);
    }

    QBuffer *buffer() { return m_buffer; }

public Q_SLOTS:
    virtual void notify(const QJsonRpcMessage &message) {
        QJsonDocument doc = QJsonDocument(message.toObject());
        QByteArray data = doc.toJson();
        m_buffer->write(data);
        m_buffer->seek(m_buffer->pos() - data.size());
        qJsonRpcDebug() << Q_FUNC_INFO << "sending: " << data;
    }

private:
    QBuffer *m_buffer;

};

class FakeQJsonRpcServerSocket : public QJsonRpcSocket
{
    Q_OBJECT
public:
    FakeQJsonRpcServerSocket(QIODevice *device, QObject *parent = 0)
        : QJsonRpcSocket(device, parent),
          m_device(device) {}

public Q_SLOTS:
    virtual void notify(const QJsonRpcMessage &message) {
        QJsonDocument doc = QJsonDocument(message.toObject());
        QByteArray data = doc.toJson();
        m_device->write(data);
        m_device->seek(m_device->pos() - data.size());
        qJsonRpcDebug() << Q_FUNC_INFO << "sending: " << data;
    }

private:
    QIODevice *m_device;

};

class FakeQJsonRpcServerPrivate : public QJsonRpcAbstractServerPrivate
{
public:
    FakeQJsonRpcServerPrivate(QJsonRpcAbstractServer *q)
        : QJsonRpcAbstractServerPrivate(q)
    {}

    virtual void _q_processIncomingConnection() {}
    virtual void _q_clientDisconnected() {}
};

class FakeQJsonRpcServer : public QJsonRpcAbstractServer
{
    Q_OBJECT
public:
    FakeQJsonRpcServer(QObject *parent = 0)
        : QJsonRpcAbstractServer(*new FakeQJsonRpcServerPrivate(this), parent),
          m_buffer(0)
    {
        m_buffer = new QBuffer(this);
        m_buffer->open(QIODevice::ReadWrite);
    }

    QBuffer *buffer() { return m_buffer; }
    void addSocket(QJsonRpcSocket *socket) {
#if QT_VERSION >= 0x050100 || QT_VERSION <= 0x050000
        socket->setWireFormat(wireFormat());
#endif
        connect(socket, SIGNAL(messageReceived(QJsonRpcMessage)),
                  this, SLOT(received(QJsonRpcMessage)));

        QJsonRpcSocket *tmpSocket = new FakeQJsonRpcServerSocket(m_buffer, this);
        d_func()->clients.append(tmpSocket);
    }

private Q_SLOTS:
    void received(const QJsonRpcMessage &message) {
        FakeQJsonRpcSocket *socket = static_cast<FakeQJsonRpcSocket*>(sender());
        if (!socket) {
            qJsonRpcDebug() << Q_FUNC_INFO << "called without service socket";
            return;
        }

        QJsonRpcSocket *returnSocket = new FakeQJsonRpcServerSocket(m_buffer, this);
        QJsonRpcServiceProvider::processMessage(returnSocket, message);
    }

private:
    virtual void processIncomingConnection() {}
    virtual QString errorString() const { return QString(); }
    virtual void clientDisconnected() {}

    QBuffer *m_buffer;
};

class TestQJsonRpcServer: public QObject
{
    Q_OBJECT
public:
    TestQJsonRpcServer();

    enum ServerType {
        TcpServer,
        LocalServer,
        BufferServer
    };

private Q_SLOTS:
    void initTestCase_data();
    void initTestCase();
    void cleanupTestCase();
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
    void clearBuffers();
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
Q_DECLARE_METATYPE(QJsonDocument::JsonFormat)

#if QT_VERSION < 0x050000
Q_DECLARE_METATYPE(QJsonArray)
#endif

void TestQJsonRpcServer::clearBuffers()
{
    FakeQJsonRpcSocket *cs = qobject_cast<FakeQJsonRpcSocket*>(clientSocket.data());
    cs->buffer()->close();
    cs->buffer()->buffer().clear();
    QVERIFY(cs->buffer()->open(QIODevice::ReadWrite));

    FakeQJsonRpcSocket *ss = qobject_cast<FakeQJsonRpcSocket*>(serverSocket.data());
    ss->buffer()->close();
    ss->buffer()->buffer().clear();
    QVERIFY(ss->buffer()->open(QIODevice::ReadWrite));

    FakeQJsonRpcServer *s = qobject_cast<FakeQJsonRpcServer*>(server.data());
    s->buffer()->close();
    s->buffer()->buffer().clear();
    QVERIFY(s->buffer()->open(QIODevice::ReadWrite));
}

class TestService : public QJsonRpcService
{
    Q_OBJECT
    Q_CLASSINFO("serviceName", "service")
public:
    TestService(QObject *parent = 0)
        : QJsonRpcService(parent),
          m_called(0)
    {}

    void resetCount() { m_called = 0; }
    int callCount() const { return m_called; }

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

    QVariantList variantListParameter(const QVariantList &data) {
        return data;
    }

    QVariant variantStringResult() {
        return "hello";
    }

    QVariantList variantListResult() {
        return QVariantList() << "one" << 2 << 3.0;
    }

    QVariantMap variantMapResult() {
        QVariantMap result;
        result["one"] = 1;
        result["two"] = 2.0;
        return result;
    }

    void increaseCalled() {
        m_called++;
    }

    /* NOTE: suppress binding warnings
    bool methodWithListOfInts(const QList<int> &list) {
        if (list.size() < 3)
            return false;
        if (list.at(0) != 300)
            return false;
        if (list.at(1) != 30)
            return false;
        if (list.at(2) != 3)
            return false;
        return true;
    }
    */

    QString variantMapInvalidParam(const QVariantMap &map) {
        return map["foo"].toString();
    }

    void outputParameter(int in1, int &out, int in2)
    {
        out = in1 + out + in2;
    }

    void outputParameterWithStrings(const QString &first, QString &output, const QString &last)
    {
        if (output.isEmpty())
            output = QString("%1 %2").arg(first).arg(last);
        else
            output.append(QString(" %1 %2").arg(first).arg(last));
    }

    bool overloadedMethod(int input) { Q_UNUSED(input) return true; }
    bool overloadedMethod(const QString &input) { Q_UNUSED(input) return false; }

    bool stringListParameter(int one, const QString &two, const QString &three, const QStringList &list)
    {
        Q_UNUSED(one);
        Q_UNUSED(two);
        Q_UNUSED(three);
        Q_UNUSED(list);
        return true;
    }

    bool delayedResponse()
    {
        QTest::qSleep(100);
        return true;
    }

private:
    int m_called;

};

TestQJsonRpcServer::TestQJsonRpcServer()
    : server(0),
      clientSocket(0),
      serverSocket(0)
{
}

void TestQJsonRpcServer::initTestCase_data()
{
#if QT_VERSION >= 0x050100 || QT_VERSION <= 0x050000
    QTest::addColumn<ServerType>("serverType");
    QTest::addColumn<QJsonDocument::JsonFormat>("wireFormat");

    QTest::newRow("buffer-indented") << BufferServer << QJsonDocument::Indented;
    QTest::newRow("buffer-compact") << BufferServer << QJsonDocument::Compact;
    QTest::newRow("tcp-indented") << TcpServer << QJsonDocument::Indented;
    QTest::newRow("tcp-compact") << TcpServer << QJsonDocument::Compact;
    QTest::newRow("local-indented") << LocalServer << QJsonDocument::Indented;
    QTest::newRow("local-compact") << LocalServer << QJsonDocument::Compact;
#else
    QTest::addColumn<ServerType>("serverType");
    QTest::newRow("buffer") << BufferServer;
    QTest::newRow("tcp") << TcpServer;
    QTest::newRow("local") << LocalServer;
#endif
}

void TestQJsonRpcServer::initTestCase()
{
    qRegisterMetaType<QJsonRpcMessage>("QJsonRpcMessage");
}

void TestQJsonRpcServer::cleanupTestCase()
{
}

void TestQJsonRpcServer::init()
{
    QFETCH_GLOBAL(ServerType, serverType);
#if QT_VERSION >= 0x050100 || QT_VERSION <= 0x050000
    QFETCH_GLOBAL(QJsonDocument::JsonFormat, wireFormat);
#endif
    if (serverType == BufferServer) {
        FakeQJsonRpcServer *s = new FakeQJsonRpcServer(this);
        FakeQJsonRpcSocket *cs = new FakeQJsonRpcSocket(s->buffer());
        FakeQJsonRpcSocket *ss = new FakeQJsonRpcSocket(cs->buffer());
        s->addSocket(ss);

        server.reset(s);
        clientSocket.reset(cs);
        serverSocket.reset(ss);
    } else {
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

#if QT_VERSION >= 0x050100 || QT_VERSION <= 0x050000
    server->setWireFormat(wireFormat);
    clientSocket->setWireFormat(wireFormat);
    if (serverSocket)
        serverSocket->setWireFormat(wireFormat);

    QCOMPARE(server->wireFormat(), wireFormat);
    QCOMPARE(clientSocket->wireFormat(), wireFormat);
#endif
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
    server->addService(new TestService);

    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));
    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.noParam");
    QJsonRpcMessage response = clientSocket->sendMessageBlocking(request);
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QCOMPARE(request.id(), response.id());
    QCOMPARE(spyMessageReceived.count(), 1);
}

void TestQJsonRpcServer::singleParameter()
{
    server->addService(new TestService);

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
    QFETCH_GLOBAL(ServerType, serverType);
    server->addService(new TestService);
    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));

    QJsonRpcMessage stringRequest = QJsonRpcMessage::createRequest("service.overloadedMethod", QString("single"));
    QJsonRpcMessage stringResponse = clientSocket->sendMessageBlocking(stringRequest);
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(stringResponse.errorCode() == QJsonRpc::NoError);
    QCOMPARE(stringRequest.id(), stringResponse.id());
    QCOMPARE(stringResponse.result().toBool(), false);

    if (serverType == BufferServer)
        clearBuffers();
    QJsonRpcMessage intRequest = QJsonRpcMessage::createRequest("service.overloadedMethod", 10);
    QJsonRpcMessage intResponse = clientSocket->sendMessageBlocking(intRequest);
    QCOMPARE(spyMessageReceived.count(), 2);
    QVERIFY(intResponse.errorCode() == QJsonRpc::NoError);
    QCOMPARE(intRequest.id(), intResponse.id());
    QCOMPARE(intResponse.result().toBool(), true);

    if (serverType == BufferServer)
        clearBuffers();
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
    server->addService(new TestService);

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
    server->addService(new TestService);

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
    server->addService(new TestService);

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
    server->addService(new TestService);

    QJsonRpcMessage response =
        clientSocket->invokeRemoteMethodBlocking("service.variantStringResult");
    QVERIFY(response.errorCode() == QJsonRpc::NoError);
    QString stringResult = response.result().toString();
    QCOMPARE(stringResult, QLatin1String("hello"));
}

void TestQJsonRpcServer::invalidArgs()
{
    server->addService(new TestService);

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
    server->addService(new TestService);

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
    server->addService(new TestService);

    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));
    QJsonRpcMessage request("{\"jsonrpc\": \"2.0\", \"id\": 666}");
    clientSocket->sendMessageBlocking(request);

    QCOMPARE(spyMessageReceived.count(), 1);
    QVariant message = spyMessageReceived.takeFirst().at(0);
    QJsonRpcMessage error = message.value<QJsonRpcMessage>();
    QCOMPARE(request.id(), error.id());
    QVERIFY(error.errorCode() == QJsonRpc::InvalidRequest);
}

void TestQJsonRpcServer::qVariantMapInvalidParam()
{
    TestService *service = new TestService;
    server->addService(service);

    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));
    const char *invalid = "{\"jsonrpc\": \"2.0\", \"id\": 0, \"method\": \"service.variantMapInvalidParam\",\"params\": [[{\"foo\":\"bar\",\"baz\":\"quux\"}, {\"foo\":\"bar\"}]]}";
    QJsonRpcMessage request(invalid);
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

    server->addService(new TestService);

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

void TestQJsonRpcServer::numberParameters()
{
    TestNumberParamsService *service = new TestNumberParamsService;
    server->addService(service);

    QJsonArray params;
    params.append(10);
    params.append(3.14159);
    QJsonRpcMessage request =
        QJsonRpcMessage::createRequest("service.numberParameters", params);
    clientSocket->sendMessageBlocking(request);
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

void TestQJsonRpcServer::hugeResponse()
{
    server->addService(new TestHugeResponseService);
    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));
    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.hugeResponse");
    QJsonRpcMessage response = clientSocket->sendMessageBlocking(request);
    QCOMPARE(spyMessageReceived.count(), 1);
    QVERIFY(response.isValid());
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

void TestQJsonRpcServer::complexMethod()
{
    server->addService(new TestComplexMethodService);
    QSignalSpy spyMessageReceived(clientSocket.data(), SIGNAL(messageReceived(QJsonRpcMessage)));
    QJsonRpcMessage request =
        QJsonRpcMessage::createRequest("service.complex.prefix.for.testMethod");
    QJsonRpcMessage response = clientSocket->sendMessageBlocking(request);
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

void TestQJsonRpcServer::defaultParameters()
{
    QFETCH_GLOBAL(ServerType, serverType);
    server->addService(new TestDefaultParametersService);

    // test without name
    QJsonRpcMessage noNameRequest =
        QJsonRpcMessage::createRequest("service.testMethod");
    QJsonRpcMessage response = clientSocket->sendMessageBlocking(noNameRequest);
    QVERIFY(response.type() != QJsonRpcMessage::Error);
    QCOMPARE(response.result().toString(), QLatin1String("empty string"));

    // test with name
    if (serverType == BufferServer)
        clearBuffers();
    QJsonRpcMessage nameRequest =
        QJsonRpcMessage::createRequest("service.testMethod", QLatin1String("matt"));
    response = clientSocket->sendMessageBlocking(nameRequest);
    QVERIFY(response.type() != QJsonRpcMessage::Error);
    QCOMPARE(response.result().toString(), QLatin1String("hello matt"));

    // test multiparameter
    if (serverType == BufferServer)
        clearBuffers();
    QJsonRpcMessage konyRequest =
        QJsonRpcMessage::createRequest("service.testMethod2", QLatin1String("KONY"));
    response = clientSocket->sendMessageBlocking(konyRequest);
    QVERIFY(response.type() != QJsonRpcMessage::Error);
    QCOMPARE(response.result().toString(), QLatin1String("KONY2012"));
}

class TestNotifyService : public QJsonRpcService
{
    Q_OBJECT
    Q_CLASSINFO("serviceName", "service")
public:
    TestNotifyService(QObject *parent = 0)
        : QJsonRpcService(parent)
    {
    }

public Q_SLOTS:
    void testMethod() { qJsonRpcDebug() << "text"; }
};

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
    server->addService(new TestService);
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
    server->addService(new TestService);

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
    server->addService(new TestService);
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

class TestServiceWithoutServiceName : public QJsonRpcService
{
    Q_OBJECT
public:
    TestServiceWithoutServiceName(QObject *parent = 0)
        : QJsonRpcService(parent)
    {}

public Q_SLOTS:
    QString testMethod(const QString &string) const {
        return string;
    }

};

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
