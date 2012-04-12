#ifndef QJSONRPCSERVICE_H
#define QJSONRPCSERVICE_H

#include <QObject>
#include <QHostAddress>

#include "qjsonrpcmessage.h"

class QJsonRpcServiceProvider;
class Q_JSONRPC_EXPORT QJsonRpcService : public QObject
{
    Q_OBJECT
public:
    explicit QJsonRpcService(QObject *parent = 0);

private:
    QJsonRpcMessage dispatch(const QJsonRpcMessage &request) const;
    void cacheInvokableInfo();
    QMultiHash<QByteArray, int> m_invokableMethodHash;
    QHash<int, QList<int> > m_parameterTypeHash;
    friend class QJsonRpcServiceProvider;
};

class Q_JSONRPC_EXPORT QJsonRpcServiceReply : public QObject
{
    Q_OBJECT
public:
    explicit QJsonRpcServiceReply(QObject *parent = 0);
    QJsonRpcMessage response() const;

Q_SIGNALS:
    void finished();

private:
    QJsonRpcMessage m_response;
    friend class QJsonRpcServiceSocket;
};

// IDEA: QJsonRpcServiceSocket inherit from QJsonRpcServiceProvider
//       QJsonRpcServiceProvider just has addService, process message
//       QJsonRpcLocalServer/QJsonRpcTcpServer inherit from QJsonRpcServiceProvider + QJsonRpcServer

class QJsonRpcServiceProviderPrivate;
class Q_JSONRPC_EXPORT QJsonRpcServiceProvider : public QObject
{
    Q_OBJECT
public:
    ~QJsonRpcServiceProvider();
    void addService(QJsonRpcService *service);

protected Q_SLOTS:
    void processMessage(const QJsonRpcMessage &message);

protected:
    explicit QJsonRpcServiceProvider(QJsonRpcServiceProviderPrivate *dd, QObject *parent);
    Q_DECLARE_PRIVATE(QJsonRpcServiceProvider)
    QScopedPointer<QJsonRpcServiceProviderPrivate> d_ptr;

};










class QJsonRpcServiceSocketPrivate;
class Q_JSONRPC_EXPORT QJsonRpcServiceSocket : public QObject
{
    Q_OBJECT
public:
    explicit QJsonRpcServiceSocket(QIODevice *device, QObject *parent = 0);
    ~QJsonRpcServiceSocket();

    bool isValid() const;

    //    void sendMessage(const QList<QJsonRpcMessage> &bulk);
    QJsonRpcServiceReply *sendMessage(const QJsonRpcMessage &message);
    QJsonRpcServiceReply *invokeRemoteMethod(const QString &method, const QVariant &arg1 = QVariant(),
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
    Q_DECLARE_PRIVATE(QJsonRpcServiceSocket)
    QScopedPointer<QJsonRpcServiceSocketPrivate> d_ptr;

};

class QJsonRpcServerPrivate;
class Q_JSONRPC_EXPORT QJsonRpcServer : public QJsonRpcServiceProvider
{
    Q_OBJECT
public:
    ~QJsonRpcServer();
    virtual QString errorString() const = 0;

public Q_SLOTS:
    void notifyConnectedClients(const QJsonRpcMessage &message);
    void notifyConnectedClients(const QString &method, const QVariantList &params = QVariantList());

private Q_SLOTS:
    virtual void processIncomingConnection() = 0;
    virtual void clientDisconnected() = 0;

protected:
    explicit QJsonRpcServer(QJsonRpcServerPrivate *dd, QObject *parent);
    Q_DECLARE_PRIVATE(QJsonRpcServer)

};

class QJsonRpcLocalServerPrivate;
class QJsonRpcLocalServer : public QJsonRpcServer
{
    Q_OBJECT
public:
    explicit QJsonRpcLocalServer(QObject *parent = 0);
    ~QJsonRpcLocalServer();

    QString errorString() const;
    bool listen(const QString &service);

private Q_SLOTS:
    void processIncomingConnection();
    void clientDisconnected();

private:
    Q_DECLARE_PRIVATE(QJsonRpcLocalServer)

};

class QJsonRpcTcpServerPrivate;
class QJsonRpcTcpServer : public QJsonRpcServer
{
    Q_OBJECT
public:
    explicit QJsonRpcTcpServer(QObject *parent = 0);
    ~QJsonRpcTcpServer();

    QString errorString() const;
    bool listen(const QHostAddress &address, quint16 port);

private Q_SLOTS:
    void processIncomingConnection();
    void clientDisconnected();

private:
    Q_DECLARE_PRIVATE(QJsonRpcTcpServer)

};

#endif

