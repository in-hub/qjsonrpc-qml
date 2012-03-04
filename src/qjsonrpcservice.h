#ifndef QJSONRPCSERVICE_H
#define QJSONRPCSERVICE_H

#include <QObject>
#include <QHash>
#include <QHostAddress>
#include <QWeakPointer>
#include <QLocalSocket>


#include "qjsonrpcmessage.h"

class Q_JSONRPC_EXPORT QJsonRpcService : public QObject
{
    Q_OBJECT
public:
    QJsonRpcService(QObject *parent = 0);
    virtual QString serviceName() const = 0;

private:
     QJsonRpcMessage dispatch(const QJsonRpcMessage &request) const;
     void cacheInvokableInfo();
     QHash<QByteArray, int> m_invokableMethodHash;
     QHash<int, QList<int> > m_parameterTypeHash;
     friend class QJsonRpcServiceProvider;
};

class Q_JSONRPC_EXPORT QJsonRpcServiceSocket : public QObject
{
    Q_OBJECT
public:
    explicit QJsonRpcServiceSocket(QLocalSocket *localSocket, QObject *parent = 0);
    ~QJsonRpcServiceSocket();

    bool isValid() const;

    void sendMessage(const QJsonRpcMessage &message);
    void sendMessage(const QList<QJsonRpcMessage> &bulk);
    void invokeRemoteMethod(const QString &method, const QVariant &arg1 = QVariant(),
                            const QVariant &arg2 = QVariant(), const QVariant &arg3 = QVariant(),
                            const QVariant &arg4 = QVariant(), const QVariant &arg5 = QVariant(),
                            const QVariant &arg6 = QVariant(), const QVariant &arg7 = QVariant(),
                            const QVariant &arg8 = QVariant(), const QVariant &arg9 = QVariant(),
                            const QVariant &arg10 = QVariant());
Q_SIGNALS:
    void messageReceived(const QJsonRpcMessage &message);

private Q_SLOTS:
    void processIncomingData();

private:
    QWeakPointer<QLocalSocket> m_socket;
};

class QLocalServer;
class Q_JSONRPC_EXPORT QJsonRpcServiceProvider : public QObject
{
    Q_OBJECT
public:
    QJsonRpcServiceProvider(QObject *parent = 0);
    ~QJsonRpcServiceProvider();

    void addService(QJsonRpcService *service);
    bool listen(const QString &name);

public Q_SLOTS:
    void notifyConnectedClients(const QJsonRpcMessage &message);

private Q_SLOTS:
    void processIncomingConnection();
    void processMessage(const QJsonRpcMessage &message);
    void clientDisconnected();

private:
    QLocalServer *m_server;
    QList<QJsonRpcServiceSocket *> m_clients;
    QHash<QLocalSocket *, QJsonRpcServiceSocket *> m_serviceSocketLookup;
    QHash<QString, QJsonRpcService *> m_services;

};

#endif

