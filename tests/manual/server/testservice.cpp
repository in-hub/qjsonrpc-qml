#include "testservice.h"

TestService::TestService(QObject *parent)
    : QJsonRpcService(parent)
{
}

TestService::~TestService()
{
}

QString TestService::serviceName() const
{
    return "agent";
}

void TestService::testMethod()
{
    qDebug() << __PRETTY_FUNCTION__ << "called";
}

void TestService::testMethodWithParams(const QString &first, bool second, int third, const QVariant &fourth)
{
    qDebug() << __PRETTY_FUNCTION__ << "with parameters: " << endl
             << " first: " << first << endl
             << "second: " << second << endl
             << " third: " << third << endl
             << "fourth: " << fourth;
}

