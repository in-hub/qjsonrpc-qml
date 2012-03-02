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

    static QJsonRpcMessage createRequest(const QString &method, const QVariantList &params);
    static QJsonRpcMessage createError(int code, const QString &message = QString(), const QVariant &data = QVariant());
    QJsonRpcMessage createResponse(const QVariant &result);
    QJsonRpcMessage createResponse(const QJsonRpcMessage &error);

    int type() const;

    // request
    QString method() const;
    QVariantList params() const;

    // response
    QVariant result() const;

    // error
    int errorCode() const;
    QString errorMessage() const;
    QVariant errorData() const;

private:
    QSharedDataPointer<QJsonRpcMessagePrivate> d;

};















class QJsonRpcMessageOLD
{
public:
    QJsonRpcMessageOLD();
    QString jsonrpc() const;

    QString id() const;
    void setId(const QString &id);

    virtual QJsonObject serialize() const;
    virtual bool parse(const QJsonObject &data);

private:
    QString m_id;

};

class QJsonRpcError
{
public:
    QJsonRpcError();
    int code() const;
    void setCode(int code);

    QString message() const;
    void setMessage(const QString &message);

    QJsonValue data() const;
    void setData(const QJsonValue &data);

public:
    QJsonObject serialize() const;
    static bool isError(const QJsonObject &data);
    bool parse(const QJsonObject &data);

private:
    int m_code;
    QString m_message;
    QJsonValue m_data;

};

class QJsonRpcRequest : public QJsonRpcMessageOLD
{
public:
    QJsonRpcRequest();
    QString method() const;
    void setMethod(const QString &method);

    QJsonArray params() const;
    void setParams(const QJsonArray &params);

public:
    QJsonObject serialize() const;
    static bool isRequest(const QJsonObject &data);
    bool parse(const QJsonObject &data);

private:
    static int uniqueRequestCounter;
    QString m_method;
    QJsonArray m_params;

};

class QJsonRpcNotification : public QJsonRpcRequest
{
public:
    QJsonRpcNotification();

public:
    QJsonObject serialize() const;
    static bool isNotification(const QJsonObject &data);
    bool parse(const QJsonObject &data);

};

class QJsonRpcResponse : public QJsonRpcMessageOLD
{
public:
    QJsonRpcResponse();
    QJsonValue result() const;
    void setResult(const QJsonValue &result);

    QJsonRpcError error() const;
    void setError(const QJsonRpcError &error);
    bool isError() const;

public:
    QJsonObject serialize() const;
    static bool isResponse(const QJsonObject &data);
    bool parse(const QJsonObject &data);

private:
    QJsonValue m_result;
    QJsonRpcError m_error;

};

class QJsonRpcService : public QObject
{
    Q_OBJECT
public:
    QJsonRpcService(QObject *parent = 0);
    virtual QString serviceName() const = 0;

private:    // these are just for ServiceManager
     QJsonValue dispatch(const QByteArray &method, const QJsonArray &args = QJsonArray()) const;
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

    void callRemoteMethod(const QString &method, const QVariant &arg1 = QVariant(),
                          const QVariant &arg2 = QVariant(), const QVariant &arg3 = QVariant(),
                          const QVariant &arg4 = QVariant(), const QVariant &arg5 = QVariant(),
                          const QVariant &arg6 = QVariant(), const QVariant &arg7 = QVariant(),
                          const QVariant &arg8 = QVariant(), const QVariant &arg9 = QVariant(),
                          const QVariant &arg10 = QVariant());

Q_SIGNALS:
    void responseReceived(const QJsonRpcResponse &response);
    void notificationReceived(const QJsonRpcNotification &notification);

private Q_SLOTS:
    void processIncomingConnection();
    void clientDisconnected();
    void processIncomingData();

private:
    QHash<QString, QJsonRpcService*> m_services;

    QLocalServer *m_server;
    QByteArray m_readBuffer;
    QList<QLocalSocket*> m_clients;

    QLocalSocket *m_client;

};


Q_DECLARE_METATYPE(QJsonRpcError)
Q_DECLARE_METATYPE(QJsonRpcResponse)
Q_DECLARE_METATYPE(QJsonRpcNotification)

#endif
