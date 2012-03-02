#ifndef QJSONRPC_H
#define QJSONRPC_H

#include <QHash>
#include <QLocalSocket>
#include <QLocalServer>
#include <QVariant>
#include <QSharedDataPointer>
#include <QMetaType>

#include "qjsonvalue.h"
#include "qjsonobject.h"
#include "qjsonarray.h"

// error codes defined by spec
namespace QJsonRpc {
    enum ErrorCode {
        ParseError      = -32700,        	// Invalid JSON was received by the server.
                                            // An error occurred on the server while parsing the JSON text.
        InvalidRequest  = -32600,         	// The JSON sent is not a valid Request object.
        MethodNotFound  = -32601,           // The method does not exist / is not available.
        InvalidParams   = -32602,           // Invalid method parameter(s).
        InternalError   = -32603,           // Internal JSON-RPC error.
        ServerErrorBase = -32000,           // Reserved for implementation-defined server-errors.
        UserError       = -32099            // Anything after this is user defined
    };
}

class QJsonRpcMessagePrivate;
class QJsonRpcMessage
{
public:
    QJsonRpcMessage();
    QJsonRpcMessage(const QJsonObject &message);
    QJsonRpcMessage(const QJsonRpcMessage &other);
    QJsonRpcMessage &operator=(const QJsonRpcMessage &other);
    ~QJsonRpcMessage();

    enum Type {
        Invalid,
        Request,
        Response,
        Notification,
        Error
    };

    static QJsonRpcMessage createRequest(const QString &method, const QVariantList &params = QVariantList());
    static QJsonRpcMessage createNotification(const QString &method, const QVariantList &params);
    QJsonRpcMessage createResponse(const QVariant &result) const;
    QJsonRpcMessage createErrorResponse(QJsonRpc::ErrorCode code, const QString &message = QString(),
                                        const QVariant &data = QVariant()) const;

    QJsonRpcMessage::Type type() const;
    int id() const;

    // request
    QString method() const;
    QVariantList params() const;

    // response
    QVariant result() const;

    // error
    int errorCode() const;
    QString errorMessage() const;
    QVariant errorData() const;

    QJsonObject toObject() const;

private:
    friend class QJsonRpcMessagePrivate;
    QSharedDataPointer<QJsonRpcMessagePrivate> d;

};

QDebug operator<<(QDebug, const QJsonRpcMessage &);


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
    QByteArray m_readBuffer;
    QList<QLocalSocket*> m_clients;

    QLocalSocket *m_client;

};

Q_DECLARE_METATYPE(QJsonRpcMessage)

#endif
