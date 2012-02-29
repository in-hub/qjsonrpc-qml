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
    void testMethodWithParams(const QString &first, bool second, int third, const QVariant &fourth);

};

#endif
