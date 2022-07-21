#ifndef QJSONRPCHTTPSERVER_H
#define QJSONRPCHTTPSERVER_H

#include <QSslConfiguration>
#include <QTcpServer>
#include <QHash>

#include "qjsonrpcabstractserver.h"
#include "qjsonrpcglobal.h"

class QJsonRpcHttpServerPrivate;

class QJSONRPC_EXPORT QJsonRpcHttpServer : public QObject, public QJsonRpcAbstractServer
{
    Q_OBJECT
public:
    QJsonRpcHttpServer(QObject *parent = 0);
    ~QJsonRpcHttpServer();

    void listen(const QString& name);
    void listen(const QHostAddress &address = QHostAddress::Any, quint16 port = 0);

    void close();

    QSslConfiguration sslConfiguration() const;
    void setSslConfiguration(const QSslConfiguration &config);

    int connectedClientCount() const override;

Q_SIGNALS:
    void clientConnected() override;
    void clientDisconnected() override;

public Q_SLOTS:
    void notifyConnectedClients(const QJsonRpcMessage &message) override;
    void notifyConnectedClients(const QString &method, const QJsonArray &params) override;

private Q_SLOTS:
    void processIncomingConnection(qintptr socketDescriptor);
    void processIncomingMessage(const QJsonRpcMessage &message);

private:
    Q_DECLARE_PRIVATE(QJsonRpcHttpServer)
    Q_DISABLE_COPY(QJsonRpcHttpServer)
    QScopedPointer<QJsonRpcHttpServerPrivate> d_ptr;

    Q_PRIVATE_SLOT(d_func(), void _q_socketDisconnected())

};

#endif
