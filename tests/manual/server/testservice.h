#ifndef TESTSERVICE_H
#define TESTSERVICE_H

#include "qjsonrpc.h"

class TestService : public QJsonRpcService
{
    Q_OBJECT
public:
    TestService(QObject *parent = 0);
    ~TestService();

    QString serviceName() const;

public Q_SLOTS:
    void testMethod();
    void testMethodWithParams(const QString &first, bool second, double third);
    void testMethodWithVariantParams(const QString &first, bool second, double third, const QVariant &fourth);
    QString testMethodWithParamsAndReturnValue(const QString &name);
    void testMethodWithDefaultParameter(const QString &first, const QString &second = QString());

};


#endif
