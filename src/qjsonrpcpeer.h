#ifndef QJSONRPCPEER_H
#define QJSONRPCPEER_H

#include <QHash>
#include <QLocalSocket>
#include <QLocalServer>
#include <QVariant>

#include "qjsonrpcmessage.h"

class QJsonRpcService : public QObject
{
    Q_OBJECT
public:
    QJsonRpcService(QObject *parent = 0);
    virtual QString serviceName() const = 0;

private:    // these are just for ServiceManager
     QVariant dispatch(const QByteArray &method, const QVariantList &args = QVariantList()) const;
     void cacheInvokableInfo();
     QHash<QByteArray, int> m_invokableMethodHash;
     QHash<int, QList<int> > m_parameterTypeHash;
     friend class QJsonRpcPeer;
};

class QJsonRpcPeer : public QObject
{
    Q_OBJECT
public:
    QJsonRpcPeer(QObject *parent = 0);
    ~QJsonRpcPeer();

    void addService(QJsonRpcService *service);

    bool listenForPeers(const QString &socket);
    void connectToPeer(const QString &socket);

    void sendMessage(const QJsonRpcMessage &message);
    void sendMessages(const QList<QJsonRpcMessage> &bulk);
    void callRemoteMethod(const QString &method, const QVariant &arg1 = QVariant(),
                          const QVariant &arg2 = QVariant(), const QVariant &arg3 = QVariant(),
                          const QVariant &arg4 = QVariant(), const QVariant &arg5 = QVariant(),
                          const QVariant &arg6 = QVariant(), const QVariant &arg7 = QVariant(),
                          const QVariant &arg8 = QVariant(), const QVariant &arg9 = QVariant(),
                          const QVariant &arg10 = QVariant());

Q_SIGNALS:
    void messageReceived(const QJsonRpcMessage &message);

private Q_SLOTS:
    void processIncomingConnection();
    void clientDisconnected();
    void processIncomingData();

private:
    void processMessage(QLocalSocket *socket, const QJsonRpcMessage &message);
    void sendMessage(QLocalSocket *socket, const QJsonRpcMessage &message);
    void sendMessages(QLocalSocket *socket, const QList<QJsonRpcMessage> &message);

    QHash<QString, QJsonRpcService*> m_services;

    QLocalServer *m_server;
    QList<QLocalSocket*> m_clients;
    QLocalSocket *m_client;

};

Q_DECLARE_METATYPE(QJsonRpcMessage)

#endif
