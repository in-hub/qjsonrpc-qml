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

#include "qjsonrpcabstractserver.h"
#include "qjsonrpcmessage.h"
#include "qjsonrpcservice.h"

class TestQJsonRpcService: public QObject
{
    Q_OBJECT  
private slots:
    void dispatch();
    void ambiguousDispatch();
    void dispatchSignals();

};

class TestService : public QJsonRpcService
{
    Q_OBJECT
    Q_CLASSINFO("serviceName", "service")
public:
    TestService(QObject *parent = 0)
        : QJsonRpcService(parent),
          m_stringCount(0),
          m_intCount(0),
          m_variantCount(0)
    {}

    bool testDispatch(const QJsonRpcMessage &message) {
        return QJsonRpcService::dispatch(message);
    }

    int stringCount() const { return m_stringCount; }
    int intCount() const { return m_intCount; }
    int variantCount() const { return m_variantCount; }
    void resetCounters() { m_stringCount = m_intCount = m_variantCount = 0; }

Q_SIGNALS:
    void testSignal();
    void testSignalWithParameter(const QString &param);

public Q_SLOTS:
    QString testMethod(const QString &string) const {
        return string;
    }

    // note: order of definition matters here for ambiguousDispatch test
    void ambiguousMethod(const QString &) {
        m_stringCount++;
    }

    void ambiguousMethod(int) {
        m_intCount++;
    }

    void ambiguousMethod(const QVariant &) {
        m_variantCount++;
    }

private:
    int m_stringCount;
    int m_intCount;
    int m_variantCount;

};

class TestServiceProvider : public QJsonRpcServiceProvider
{
public:
    TestServiceProvider() {}
};

void TestQJsonRpcService::dispatch()
{
    TestServiceProvider provider;
    TestService service;
    provider.addService(&service);

    QJsonRpcMessage validRequestDispatch =
        QJsonRpcMessage::createRequest("service.testMethod", QLatin1String("testParam"));
    QVERIFY(service.testDispatch(validRequestDispatch));

    QJsonObject namedParameters;
    namedParameters.insert("string", QLatin1String("testParam"));
    QJsonRpcMessage validRequestDispatchWithNamedParameters =
        QJsonRpcMessage::createRequest("service.testMethod", namedParameters);
    QVERIFY(service.testDispatch(validRequestDispatchWithNamedParameters));

    QJsonObject invalidNamedParameters;
    invalidNamedParameters.insert("testParameter", QLatin1String("testParam"));
    QJsonRpcMessage invalidRequestDispatchWithNamedParameters =
        QJsonRpcMessage::createRequest("service.testMethod", invalidNamedParameters);
    QVERIFY(!service.testDispatch(invalidRequestDispatchWithNamedParameters));

    QJsonRpcMessage validNotificationDispatch =
        QJsonRpcMessage::createNotification("service.testMethod", QLatin1String("testParam"));
    QVERIFY(service.testDispatch(validNotificationDispatch));

    QJsonRpcMessage validNotificationDispatchWithNamedParameters =
        QJsonRpcMessage::createNotification("service.testMethod", namedParameters);
    QVERIFY(service.testDispatch(validNotificationDispatchWithNamedParameters));

    QJsonRpcMessage invalidResponseDispatch =
        validRequestDispatch.createResponse(QLatin1String("testResult"));
    QVERIFY(!service.testDispatch(invalidResponseDispatch));

    QJsonRpcMessage invalidDispatch;
    QVERIFY(!service.testDispatch(invalidDispatch));
}

void TestQJsonRpcService::ambiguousDispatch()
{
    TestServiceProvider provider;
    TestService service;
    provider.addService(&service);

    QJsonRpcMessage stringDispatch =
        QJsonRpcMessage::createRequest("service.ambiguousMethod", QLatin1String("testParam"));
    service.testDispatch(stringDispatch);
    QCOMPARE(service.stringCount(), 1);
    QCOMPARE(service.intCount(), 0);
    QCOMPARE(service.variantCount(), 0);

    QJsonRpcMessage intDispatch =
        QJsonRpcMessage::createRequest("service.ambiguousMethod", 10);
    service.testDispatch(intDispatch);
    QCOMPARE(service.stringCount(), 1);
    QCOMPARE(service.intCount(), 1);
    QCOMPARE(service.variantCount(), 0);

    QStringList stringList = QStringList() << "test" << "string" << "list";
    QJsonRpcMessage stringListDispatch =
        QJsonRpcMessage::createRequest("service.ambiguousMethod", QJsonValue::fromVariant(stringList));
    service.testDispatch(stringListDispatch);
    QCOMPARE(service.stringCount(), 1);
    QCOMPARE(service.intCount(), 1);
    QCOMPARE(service.variantCount(), 1);
}

void TestQJsonRpcService::dispatchSignals()
{
    TestServiceProvider provider;
    TestService service;
    provider.addService(&service);

    QJsonRpcMessage validRequestSignalDispatch =
        QJsonRpcMessage::createRequest("service.testSignal");
    QVERIFY(service.testDispatch(validRequestSignalDispatch));

    QJsonRpcMessage validRequestSignalWithParamDispatch =
        QJsonRpcMessage::createRequest("service.testSignalWithParameter", QLatin1String("testParam"));
    QVERIFY(service.testDispatch(validRequestSignalWithParamDispatch));

    QJsonRpcMessage invalidRequestSignalDispatch =
        QJsonRpcMessage::createRequest("service.testSignal", QLatin1String("testParam"));
    QCOMPARE(service.testDispatch(invalidRequestSignalDispatch), false);
}

QTEST_MAIN(TestQJsonRpcService)
#include "tst_qjsonrpcservice.moc"
