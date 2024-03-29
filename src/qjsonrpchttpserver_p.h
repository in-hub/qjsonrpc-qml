#ifndef QJSONRPCHTTPSERVER_P_H
#define QJSONRPCHTTPSERVER_P_H

#include <QHash>
#include <QSslSocket>
#include <QSslConfiguration>

#include "qjsonrpcsocket.h"
#include "qjsonrpcmessage.h"
#include "qjsonrpcabstractserver_p.h"

#include "http_parser.h"

class QJsonRpcTcpServer;
class QJsonRpcLocalServer;

class QJsonRpcHttpServerRpcSocket : public QJsonRpcSocket
{
public:
    explicit QJsonRpcHttpServerRpcSocket(QIODevice *device, QObject *parent = 0);
};

class QAbstractSocket;
class QJsonRpcHttpServerSocket : public QIODevice
{
    Q_OBJECT
public:
    explicit QJsonRpcHttpServerSocket(QIODevice *device, QObject *parent = 0);
    ~QJsonRpcHttpServerSocket();

    void sendErrorResponse(int statusCode);
    void sendOptionsResponse(int statusCode);

Q_SIGNALS:
    void messageReceived(const QJsonRpcMessage &message);
    void disconnected();

protected:
    qint64 bytesAvailable() const override;
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;

private Q_SLOTS:
    void readIncomingData();

private:
    static int onMessageBegin(http_parser *parser);
    static int onUrl(http_parser *parser, const char *at, size_t length);
    static int onHeaderField(http_parser *parser, const char *at, size_t length);
    static int onHeaderValue(http_parser *parser, const char *at, size_t length);
    static int onHeadersComplete(http_parser *parser);
    static int onBody(http_parser *parser, const char *at, size_t length);
    static int onMessageComplete(http_parser *parser);

private:
    Q_DISABLE_COPY(QJsonRpcHttpServerSocket)

    QIODevice *m_device;

    // request
    QByteArray m_requestPayload;
    http_parser *m_requestParser;
    http_parser_settings m_requestParserSettings;

    // for header processing
    QHash<QString, QString> m_requestHeaders;
    QString m_currentHeaderField;
    QString m_currentHeaderValue;

    // response
    QByteArray m_responseBuffer;

};

class QJsonRpcHttpServer;
class QJsonRpcHttpServerPrivate : public QJsonRpcAbstractServerPrivate
{
public:
    QJsonRpcHttpServerPrivate(QJsonRpcHttpServer *qq)
        : q_ptr(qq)
    {
    }

    // slots
    void _q_socketDisconnected();

    QJsonRpcTcpServer *tcpServer{nullptr};
    QJsonRpcLocalServer *localServer{nullptr};

    QHash<QJsonRpcHttpServerSocket*, QJsonRpcHttpServerRpcSocket*> requestSocketLookup;
    QSslConfiguration sslConfiguration;

    QJsonRpcHttpServer * const q_ptr;
    Q_DECLARE_PUBLIC(QJsonRpcHttpServer)
};

#endif
