#ifndef QJSONRPCABSTRACTSERVER_H
#define QJSONRPCABSTRACTSERVER_H

#include <QScopedPointer>

#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#else
#include "json/qjsondocument.h"
#endif

#include "qjsonrpcmessage.h"
#include "qjsonrpc_export.h"

class QJsonRpcService;
class QJsonRpcSocket;
class QJsonRpcServiceProviderPrivate;
class QJSONRPC_EXPORT QJsonRpcServiceProvider
{
public:
    ~QJsonRpcServiceProvider();
    virtual bool addService(QJsonRpcService *service);
    virtual bool removeService(QJsonRpcService *service);

protected:
    QJsonRpcServiceProvider();
    void processMessage(QJsonRpcSocket *socket, const QJsonRpcMessage &message);

private:
    Q_DECLARE_PRIVATE(QJsonRpcServiceProvider)
    QScopedPointer<QJsonRpcServiceProviderPrivate> d_ptr;

};

class QJsonRpcAbstractServerPrivate;
class QJSONRPC_EXPORT QJsonRpcAbstractServer : public QObject,
                                               public QJsonRpcServiceProvider
{
    Q_OBJECT
public:
    virtual ~QJsonRpcAbstractServer();
    virtual QString errorString() const = 0;
    virtual bool addService(QJsonRpcService *service);
    virtual bool removeService(QJsonRpcService *service);

#if QT_VERSION >= 0x050100 || QT_VERSION <= 0x050000
    QJsonDocument::JsonFormat wireFormat() const;
    void setWireFormat(QJsonDocument::JsonFormat format);
#endif

public Q_SLOTS:
    void notifyConnectedClients(const QJsonRpcMessage &message);
    void notifyConnectedClients(const QString &method, const QVariantList &params = QVariantList());

protected Q_SLOTS:
    virtual void processIncomingConnection() = 0;
    virtual void clientDisconnected() = 0;
    void processMessage(const QJsonRpcMessage &message);

protected:
    explicit QJsonRpcAbstractServer(QJsonRpcAbstractServerPrivate *dd, QObject *parent);
    Q_DECLARE_PRIVATE(QJsonRpcAbstractServer)
    QScopedPointer<QJsonRpcAbstractServerPrivate> d_ptr;

};

#endif
