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
#include <QLocalServer>
#include <QLocalSocket>

#include <QtCore/QEventLoop>
#include <QtCore/QVariant>
#include <QtTest/QtTest>

#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#else
#include "json/qjsondocument.h"
#endif

#include "qjsonrpcservice_p.h"
#include "qjsonrpcservice.h"
#include "qjsonrpcmessage.h"
#include "qjsonrpcservicereply.h"
#include "qjsonrpcsocket_p.h"
#include "qjsonrpcsocket.h"

class QBufferBackedQJsonRpcSocketPrivate : public QJsonRpcSocketPrivate
{
public:
    QBufferBackedQJsonRpcSocketPrivate(QBuffer *b)
        : buffer(b)
    {
        device = b;
    }

    virtual void _q_processIncomingData() {
        buffer->seek(0);
        QJsonRpcSocketPrivate::_q_processIncomingData();
    }

    QBuffer *buffer;

};

class QBufferBackedQJsonRpcSocket : public QJsonRpcSocket
{
    Q_OBJECT
public:
    QBufferBackedQJsonRpcSocket(QBuffer *buffer, QObject *parent = 0)
        : QJsonRpcSocket(*new QBufferBackedQJsonRpcSocketPrivate(buffer), parent)
    {
    }
};

class TestQJsonRpcSocket: public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void noParameters();
    void multiParameter();
    void notification();
    void response();
    void delayedMessageReceive();

private:
    // benchmark parsing speed
    void jsonParsingBenchmark();
};

void TestQJsonRpcSocket::initTestCase()
{
    qRegisterMetaType<QJsonRpcMessage>("QJsonRpcMessage");
}

void TestQJsonRpcSocket::cleanupTestCase()
{
}

void TestQJsonRpcSocket::init()
{
}

void TestQJsonRpcSocket::cleanup()
{
}

void TestQJsonRpcSocket::noParameters()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QJsonRpcSocket serviceSocket(&buffer, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));
    QVERIFY(serviceSocket.isValid());

    QJsonRpcMessage request = QJsonRpcMessage::createRequest(QString("test.noParam"));

    QScopedPointer<QJsonRpcServiceReply> reply;
    reply.reset(serviceSocket.sendMessage(request));

    QJsonRpcMessage bufferMessage(buffer.data());
    QCOMPARE(request.id(), bufferMessage.id());
    QCOMPARE(request.type(), bufferMessage.type());
    QCOMPARE(request.method(), bufferMessage.method());
    QCOMPARE(request.params(), bufferMessage.params());
    QCOMPARE(spyMessageReceived.count(), 0);
}

void TestQJsonRpcSocket::multiParameter()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QJsonRpcSocket serviceSocket(&buffer, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));
    QVERIFY(serviceSocket.isValid());

    QJsonArray params;
    params.append(false);
    params.append(true);
    QJsonRpcMessage request =
        QJsonRpcMessage::createRequest("test.multiParam", params);

    QScopedPointer<QJsonRpcServiceReply> reply;
    reply.reset(serviceSocket.sendMessage(request));

    QJsonRpcMessage bufferMessage(buffer.data());
    QCOMPARE(request.id(), bufferMessage.id());
    QCOMPARE(request.type(), bufferMessage.type());
    QCOMPARE(request.method(), bufferMessage.method());
    QCOMPARE(request.params(), bufferMessage.params());
    QCOMPARE(spyMessageReceived.count(), 0);
}

void TestQJsonRpcSocket::notification()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QJsonRpcSocket serviceSocket(&buffer, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));
    QVERIFY(serviceSocket.isValid());

    QJsonRpcMessage notification =
        QJsonRpcMessage::createNotification("test.notify");

    QScopedPointer<QJsonRpcServiceReply> reply;
    reply.reset(serviceSocket.sendMessage(notification));

    QJsonRpcMessage bufferMessage(buffer.data());
    QCOMPARE(notification.id(), bufferMessage.id());
    QCOMPARE(notification.type(), bufferMessage.type());
    QCOMPARE(notification.method(), bufferMessage.method());
    QCOMPARE(notification.params(), bufferMessage.params());
    QCOMPARE(spyMessageReceived.count(), 0);
}

void TestQJsonRpcSocket::response()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QJsonRpcSocket serviceSocket(&buffer, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));
    QVERIFY(serviceSocket.isValid());

    QJsonRpcMessage response = QJsonRpcMessage::createRequest("test.response");
    response = response.createResponse(QJsonValue());

    QScopedPointer<QJsonRpcServiceReply> reply;
    reply.reset(serviceSocket.sendMessage(response));

    QJsonRpcMessage bufferMessage(buffer.data());
    QCOMPARE(response.id(), bufferMessage.id());
    QCOMPARE(response.type(), bufferMessage.type());
    QCOMPARE(response.method(), bufferMessage.method());
    QCOMPARE(response.params(), bufferMessage.params());
    QCOMPARE(spyMessageReceived.count(), 0);
}

void TestQJsonRpcSocket::jsonParsingBenchmark()
{
    QFile testData(":/testwire.json");
    QVERIFY(testData.open(QIODevice::ReadOnly));
    QByteArray jsonData = testData.readAll();
    QJsonRpcSocketPrivate socketPrivate;

    int messageCount = 0;
    while (!jsonData.isEmpty()) {
        int pos = 0;
        QBENCHMARK {
            pos = socketPrivate.findJsonDocumentEnd(jsonData);
        }

        if (pos > -1) {
            messageCount++;
            jsonData = jsonData.mid(pos + 1);
        } else {
            break;
        }
    }

    QCOMPARE(messageCount, 8);
}

void TestQJsonRpcSocket::delayedMessageReceive()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QBufferBackedQJsonRpcSocket serviceSocket(&buffer, this);
    QSignalSpy spyMessageReceived(&serviceSocket,
                                  SIGNAL(messageReceived(QJsonRpcMessage)));
    QVERIFY(serviceSocket.isValid());

    QJsonRpcMessage request =
        QJsonRpcMessage::createRequest("test.multiParam");

    QJsonRpcMessage response = serviceSocket.sendMessageBlocking(request, 1);
    QVERIFY(response.type() == QJsonRpcMessage::Error);
    spyMessageReceived.removeLast();

    // this should cause a crash, before the fix
    const char *fakeDelayedResult =
        "{" \
            "\"id\": %1," \
            "\"jsonrpc\": \"2.0\"," \
            "\"result\": true" \
        "}";

    buffer.write(QString(fakeDelayedResult).arg(request.id()).toLatin1());
    while (!spyMessageReceived.size())
        qApp->processEvents();
}

QTEST_MAIN(TestQJsonRpcSocket)
#include "tst_qjsonrpcsocket.moc"
