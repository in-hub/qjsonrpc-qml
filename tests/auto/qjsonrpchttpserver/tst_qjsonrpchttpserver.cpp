#include <QtCore/QEventLoop>
#include <QtCore/QVariant>
#include <QtTest/QtTest>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#else
#include "json/qjsondocument.h"
#endif

#include "qjsonrpchttpclient.h"
#include "qjsonrpcservice.h"
#include "qjsonrpchttpserver.h"
#include "qjsonrpcmessage.h"

class TestQJsonRpcHttpServer: public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void quickTest();

private:
    // temporarily disabled
    void sslTest();

private:
    QSslConfiguration serverSslConfiguration;
    QSslConfiguration clientSslConfiguration;

};

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
    int callCount() const {
        return m_called;
    }

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

private:
    int m_called;
};

void TestQJsonRpcHttpServer::initTestCase()
{
    // setup ssl configuration for tests
    QList<QSslCertificate> caCerts =
        QSslCertificate::fromPath(QLatin1String(":/certs/qt-test-server-cacert.pem"));
    serverSslConfiguration.setCaCertificates(caCerts);
    serverSslConfiguration.setProtocol(QSsl::AnyProtocol);
}

void TestQJsonRpcHttpServer::quickTest()
{
    QJsonRpcHttpServer server;
    server.addService(new TestService);
    QVERIFY(server.listen(QHostAddress::LocalHost, 8118));

    QJsonRpcHttpClient client;
    client.setEndPoint("http://127.0.0.1:8118");

    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.noParam");
    QJsonRpcMessage response = client.sendMessageBlocking(request);
    QVERIFY(response.type() != QJsonRpcMessage::Error);
    QCOMPARE(request.id(), response.id());
}

void TestQJsonRpcHttpServer::sslTest()
{
    QJsonRpcHttpServer server;
    server.setSslConfiguration(serverSslConfiguration);
    server.addService(new TestService);
    server.listen(QHostAddress::LocalHost, 8118);

    QJsonRpcHttpClient client;
    client.setEndPoint("http://127.0.0.1:8118");
    client.setSslConfiguration(serverSslConfiguration);

    QJsonRpcMessage request = QJsonRpcMessage::createRequest("service.noParam");
    QJsonRpcMessage response = client.sendMessageBlocking(request);
    qDebug() << response;
    QVERIFY(response.type() != QJsonRpcMessage::Error);
    QCOMPARE(request.id(), response.id());
}

QTEST_MAIN(TestQJsonRpcHttpServer)
#include "tst_qjsonrpchttpserver.moc"
