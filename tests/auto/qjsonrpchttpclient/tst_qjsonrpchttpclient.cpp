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
#include <QtTest/QtTest>

#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#else
#include "json/qjsondocument.h"
#endif

#include "testhttpserver.h"
#include "qjsonrpcmessage.h"
#include "qjsonrpchttpclient.h"

#if QT_VERSION < 0x050000
template <typename T>
struct QScopedPointerObjectDeleteLater
{
    static inline void cleanup(T *pointer) { if (pointer) pointer->deleteLater(); }
};

class QObject;
typedef QScopedPointerObjectDeleteLater<QObject> QScopedPointerDeleteLater;
#endif

class TestQJsonRpcHttpClient : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();

    void basicRequest();
};

void TestQJsonRpcHttpClient::init()
{
}

void TestQJsonRpcHttpClient::cleanup()
{
}

class JsonRpcRequestHandler : public TestHttpServerRequestHandler
{
public:
    virtual QByteArray handleRequest(QNetworkAccessManager::Operation operation,
                                     const QNetworkRequest &request, const QByteArray &body)
    {
        Q_UNUSED(operation)
        Q_UNUSED(request)
        QJsonRpcMessage requestMessage(body);
        QJsonRpcMessage responseMessage =
            requestMessage.createResponse(QLatin1String("some response data"));
        QByteArray responseData = responseMessage.toJson();

        QByteArray reply;
        reply += "HTTP/1.0 200\r\n";
        reply += "Content-Type: application/json\r\n";
        reply += "Content-length: " + QByteArray::number(responseData.size()) + "\r\n";
        reply += "\r\n";
        reply += responseData;
        return reply;
    }
};

void TestQJsonRpcHttpClient::basicRequest()
{
    TestHttpServer server;
    server.setRequestHandler(new JsonRpcRequestHandler);
    QVERIFY(server.listen());

    QString url =
        QString("%1://localhost:%2").arg("http").arg(server.serverPort());
    QJsonRpcHttpClient client(url);
    QJsonRpcMessage message = QJsonRpcMessage::createRequest("testMethod");
    QJsonRpcMessage response = client.sendMessageBlocking(message);
    QVERIFY(response.type() != QJsonRpcMessage::Error);
    QCOMPARE(response.result().toString(), QLatin1String("some response data"));
}

QTEST_MAIN(TestQJsonRpcHttpClient)
#include "tst_qjsonrpchttpclient.moc"
