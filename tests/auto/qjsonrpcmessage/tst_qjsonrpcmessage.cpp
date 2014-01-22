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
#include <QtCore/QVariant>
#include <QtTest/QtTest>

#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#else
#include "json/qjsondocument.h"
#endif

#include "qjsonrpcmessage.h"

class TestQJsonRpcMessage: public QObject
{
    Q_OBJECT  
private slots:
    void testInvalidData();
    void testInvalidDataResponseWithId();
    void testInvalidDataResponseWithoutId();
    void testResponseSameId();
    void testNotificationNoId();
    void testMessageTypes();
    void testPositionalParameters();
    void testEquivalence_data();
    void testEquivalence();
    void testWithVariantListArgs();
};

void TestQJsonRpcMessage::testInvalidData()
{
    QJsonObject invalidData;
    QJsonRpcMessage message(invalidData);
    QCOMPARE(message.type(), QJsonRpcMessage::Invalid);
}

void TestQJsonRpcMessage::testInvalidDataResponseWithId()
{
    // invalid with id
    const char *invalid = "{\"jsonrpc\": \"2.0\", \"params\": [], \"id\": 666}";
    QJsonRpcMessage request(invalid);
    QJsonRpcMessage error =
        request.createErrorResponse(QJsonRpc::NoError, QString());
    QJsonRpcMessage response = request.createResponse(QString());
    QCOMPARE(request.type(), QJsonRpcMessage::Invalid);
    QCOMPARE(response.id(), request.id());
    QCOMPARE(error.type(), QJsonRpcMessage::Error);
}

void TestQJsonRpcMessage::testInvalidDataResponseWithoutId()
{
    // invalid without id
    const char *invalid = "{\"jsonrpc\": \"2.0\", \"params\": []}";
    QJsonRpcMessage request(invalid);
    QJsonRpcMessage error =
        request.createErrorResponse(QJsonRpc::NoError, QString());
    QJsonRpcMessage response = request.createResponse(QString());
    QCOMPARE(request.type(), QJsonRpcMessage::Invalid);
    QCOMPARE(response.type(), QJsonRpcMessage::Invalid);    
    QCOMPARE(error.id(), 0);
}

void TestQJsonRpcMessage::testResponseSameId()
{
    QJsonRpcMessage request =
        QJsonRpcMessage::createRequest("testMethod");
    QJsonRpcMessage response =
        request.createResponse(QLatin1String("testResponse"));
    QCOMPARE(response.id(), request.id());
}

void TestQJsonRpcMessage::testNotificationNoId()
{
    QJsonRpcMessage notification =
        QJsonRpcMessage::createNotification("testNotification");
    QCOMPARE(notification.id(), -1);
}

void TestQJsonRpcMessage::testMessageTypes()
{
    QJsonRpcMessage invalid;
    QCOMPARE(invalid.type(), QJsonRpcMessage::Invalid);

    QJsonRpcMessage request =
        QJsonRpcMessage::createRequest("testMethod");
    QCOMPARE(request.type(), QJsonRpcMessage::Request);

    QJsonRpcMessage response =
        request.createResponse(QLatin1String("testResponse"));
    QCOMPARE(response.type(), QJsonRpcMessage::Response);

    QJsonRpcMessage error = request.createErrorResponse(QJsonRpc::NoError);
    QCOMPARE(error.type(), QJsonRpcMessage::Error);

    QJsonRpcMessage notification =
        QJsonRpcMessage::createNotification("testNotification");
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

void TestQJsonRpcMessage::testEquivalence_data()
{
    QTest::addColumn<QJsonRpcMessage>("lhs");
    QTest::addColumn<QJsonRpcMessage>("rhs");
    QTest::addColumn<bool>("equal");

    QJsonRpcMessage invalid;
    {
        // REQUESTS
        const char *simpleData =
            "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"request\" }";
        QJsonRpcMessage simpleRequest(simpleData);
        QJsonRpcMessage simpleRequestCopyCtor(simpleRequest);
        QJsonRpcMessage simpleRequestEqualsOp = simpleRequest;

        const char *withParametersData =
            "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"request\", \"params\": [\"with\", \"parameters\"]}";
        QJsonRpcMessage simpleRequestWithParameters(withParametersData);

        const char *withNamedParametersData =
            "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"request\", \"params\": {\"firstName\": \"yogi\", \"lastName\": \"thebear\"}}";
        QJsonRpcMessage simpleRequestWithNamedParameters(withNamedParametersData);

        QTest::newRow("simpleRequestCopiesEqual_1") << simpleRequest << simpleRequestCopyCtor << true;
        QTest::newRow("simpleRequestCopiesEqual_2") << simpleRequest << simpleRequestEqualsOp << true;
        QTest::newRow("simpleRequestAndSimpleRequestWithParamsNotEqual") << simpleRequest
            << simpleRequestWithParameters << false;
        QTest::newRow("simpleRequestAndSimpleRequestWithNamedParamsNotEqual") << simpleRequest
            << simpleRequestWithNamedParameters << false;
        QTest::newRow("requestWithParamsNotEqualWithNamedParameters")
            << simpleRequestWithParameters << simpleRequestWithNamedParameters << false;
        QTest::newRow("simpleRequestNotEqualInvalid") << simpleRequest << invalid << false;
    }

    {
        // NOTIFICATIONS
        QJsonRpcMessage simpleNotification = QJsonRpcMessage::createNotification("notification");
        QJsonRpcMessage simpleNotificationCopyCtor(simpleNotification);
        QJsonRpcMessage simpleNotificationEqualsOp = simpleNotification;

        QJsonArray params;
        params.append(QLatin1String("yogi"));
        params.append(QLatin1String("thebear"));
        QJsonRpcMessage simpleNotificationWithParams =
            QJsonRpcMessage::createNotification("notification", params);

        QJsonObject namedParameters;
        namedParameters.insert("firstName", QLatin1String("yogi"));
        namedParameters.insert("lastName", QLatin1String("thebear"));
        QJsonRpcMessage simpleNotificationWithNamedParameters =
            QJsonRpcMessage::createNotification("notification", namedParameters);

        QTest::newRow("simpleNotificationCopiesEqual_1")
            << simpleNotification << simpleNotificationCopyCtor << true;
        QTest::newRow("simpleNotificationCopiesEqual_2")
            << simpleNotification << simpleNotificationEqualsOp << true;
        QTest::newRow("simpleNotificationNotEqualNotificationWithParams")
            << simpleNotification << simpleNotificationWithParams << false;
        QTest::newRow("simpleNotificationNotEqualNotificationWithNamedParameters")
            << simpleNotification << simpleNotificationWithNamedParameters << false;
        QTest::newRow("notificationWithParamsNotEqualWithNamedParameters")
            << simpleNotificationWithParams << simpleNotificationWithNamedParameters << false;
        QTest::newRow("simpleNotificationNotEqualInvalid") << simpleNotification << invalid << false;
    }
}

void TestQJsonRpcMessage::testEquivalence()
{
    QFETCH(QJsonRpcMessage, lhs);
    QFETCH(QJsonRpcMessage, rhs);
    QFETCH(bool, equal);

    if (equal)
        QCOMPARE(lhs, rhs);
    else
        QVERIFY(lhs != rhs);
}

void TestQJsonRpcMessage::testWithVariantListArgs()
{
    const char *varListArgsFormat = "{ " \
            "\"id\": %1, " \
            "\"jsonrpc\": \"2.0\", " \
            "\"method\": \"service.variantListParameter\", " \
            "\"params\": [[ 1, 20, \"hello\", false ]] " \
            "}";

    QVariantList firstParameter;
    firstParameter << 1 << 20 << "hello" << false;

    QJsonArray params;
    params.append(QJsonArray::fromVariantList(firstParameter));
    QJsonRpcMessage requestFromQJsonRpc =
        QJsonRpcMessage::createRequest("service.variantListParameter", params);

    // QJsonRpcMessage::createRequest is creating objects with an unique id,
    // and to allow a random test execution order - json data must have the same id
    int id = requestFromQJsonRpc.id();
    QByteArray varListArgs = QString(varListArgsFormat).arg(id).toLatin1();

    QJsonRpcMessage requestFromData(varListArgs);
    QCOMPARE(requestFromQJsonRpc, requestFromData);
}

QTEST_MAIN(TestQJsonRpcMessage)
#include "tst_qjsonrpcmessage.moc"
