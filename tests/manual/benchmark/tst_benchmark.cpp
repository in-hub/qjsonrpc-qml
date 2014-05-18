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
#include <QScopedPointer>

#include <QtCore/QEventLoop>
#include <QtCore/QVariant>
#include <QtTest/QtTest>
#include <QElapsedTimer>
#include <QThread>

#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#else
#include "json/qjsondocument.h"
#endif

#include "qjsonrpcabstractserver_p.h"
#include "qjsonrpcabstractserver.h"
#include "qjsonrpcsocket.h"
#include "qjsonrpcservice_p.h"
#include "qjsonrpcservice.h"
#include "qjsonrpcmessage.h"

class TestBenchmark: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void simpleCall();
    void namedParamsCall();

private:
    QThread::Priority m_prio;
};

class TestService : public QJsonRpcService
{
    Q_OBJECT
    Q_CLASSINFO("serviceName", "service")
public:
    TestService(QObject *parent = 0) : QJsonRpcService(parent)
    {}

    bool testDispatch(const QJsonRpcMessage &message) {
        return QJsonRpcService::dispatch(message);
    }

public Q_SLOTS:
    QString singleParam(int i) const { return QString::number(i); }
    QString singleParam(const QString &string) const { return string; }
    QString singleParam(const QVariant &k) const { return k.toString(); }

    QString namedParams(int integer, const QString &string, double doub)
    {
        (void) integer;
        (void) doub;

        return string;
    }
};

class TestServiceProvider : public QJsonRpcServiceProvider
{
public:
    TestServiceProvider() {}
};

void TestBenchmark::initTestCase()
{
    m_prio = thread()->priority();
    thread()->setPriority(QThread::TimeCriticalPriority);
}

void TestBenchmark::cleanupTestCase()
{
    thread()->setPriority(m_prio);
}

#define BENCH_LOOP_COUNT 1000000

void TestBenchmark::simpleCall()
{
    TestServiceProvider provider;
    TestService service;
    provider.addService(&service);

    QElapsedTimer timer;

    QJsonRpcMessage request = QJsonRpcMessage::createRequest(
                "service.singleParam", QString("test"));
    qDebug() << "Running preloop";
    /* Let's hotten the CPU */
    for (int i = 0; i < 1000; ++i)
        QVERIFY(service.testDispatch(request));

    qDebug() << "Starting benchmark";
    /* Clear the event queue */
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    timer.start();

    for (int i = 0; i < BENCH_LOOP_COUNT; ++i)
        QVERIFY(service.testDispatch(request));

    qint64 elapsed = timer.elapsed();
    qDebug() << elapsed;
}

void TestBenchmark::namedParamsCall()
{
    TestServiceProvider provider;
    TestService service;
    provider.addService(&service);

    QElapsedTimer timer;

    QJsonObject obj;
    obj["integer"] = 1;
    obj["string"] = QLatin1String("str");
    obj["doub"] = 1.2;
    QJsonRpcMessage request = QJsonRpcMessage::createRequest(
                "service.namedParams", obj);

    qDebug() << "Running preloop";
    /* Let's hotten the CPU */
    for (int i = 0; i < 1000; ++i)
        QVERIFY(service.testDispatch(request));

    qDebug() << "Starting benchmark";
    /* Clear the event queue */
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    timer.start();

    for (int i = 0; i < BENCH_LOOP_COUNT; ++i)
        QVERIFY(service.testDispatch(request));

    qint64 elapsed = timer.elapsed();
    qDebug() << elapsed;
}

QTEST_MAIN(TestBenchmark)
#include "tst_benchmark.moc"

