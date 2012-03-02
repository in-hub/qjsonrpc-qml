#include <QMetaObject>
#include <QMetaProperty>
#include <QMetaMethod>
#include <QStringList>
#include <QVarLengthArray>

#include "qjsondocument.h"
#include "qjsonrpc_p.h"
#include "qjsonrpc.h"

int QJsonRpcMessagePrivate::uniqueRequestCounter = 0;
QJsonRpcMessagePrivate::QJsonRpcMessagePrivate()
    : type(QJsonRpcMessage::Invalid),
      object(0)
{
}

QJsonRpcMessagePrivate::~QJsonRpcMessagePrivate()
{
    if (object)
        delete object;
}

QJsonRpcMessage::QJsonRpcMessage()
    : d(new QJsonRpcMessagePrivate)
{
}

QJsonRpcMessage::QJsonRpcMessage(const QJsonRpcMessage &other)
    : d(other.d)
{
}

QJsonRpcMessage::~QJsonRpcMessage()
{
}

QJsonRpcMessage &QJsonRpcMessage::operator=(const QJsonRpcMessage &other)
{
    d = other.d;
    return *this;
}

QJsonRpcMessage::QJsonRpcMessage(const QJsonObject &message)
    : d(new QJsonRpcMessagePrivate)
{
    if (message.contains("id")) {
        if (message.contains("result") || message.contains("error")) {
            d->object = new QJsonObject(message);
            if (message.contains("error"))
                d->type = QJsonRpcMessage::Error;
            else
                d->type = QJsonRpcMessage::Response;
        } else if (message.contains("method")) {
            d->object = new QJsonObject(message);
            d->type = QJsonRpcMessage::Request;
        }
    } else {
        if (message.contains("method")) {
            d->object = new QJsonObject(message);
            d->type = QJsonRpcMessage::Notification;
        }
    }
}

QJsonObject QJsonRpcMessage::toObject() const
{
    if (d->object)
        return QJsonObject(*d->object);
    return QJsonObject();
}

QJsonRpcMessage::Type QJsonRpcMessage::type() const
{
    return d->type;
}

QJsonRpcMessage QJsonRpcMessagePrivate::createBasicRequest(const QString &method, const QVariantList &params)
{
    QJsonRpcMessage request;
    QJsonObject *object = new QJsonObject;
    object->insert("jsonrpc", QLatin1String("2.0"));
    object->insert("method", method);
    if (!params.isEmpty())
        object->insert("params", QJsonArray::fromVariantList(params));
    request.d->object = object;
    return request;
}

QJsonRpcMessage QJsonRpcMessage::createRequest(const QString &method, const QVariantList &params)
{
    QJsonRpcMessage request = QJsonRpcMessagePrivate::createBasicRequest(method, params);
    request.d->type = QJsonRpcMessage::Request;
    QJsonRpcMessagePrivate::uniqueRequestCounter++;
    request.d->object->insert("id", QJsonRpcMessagePrivate::uniqueRequestCounter);
    return request;
}

QJsonRpcMessage QJsonRpcMessage::createNotification(const QString &method, const QVariantList &params)
{
    QJsonRpcMessage notification = QJsonRpcMessagePrivate::createBasicRequest(method, params);
    notification.d->type = QJsonRpcMessage::Notification;
    return notification;
}

QJsonRpcMessage QJsonRpcMessage::createResponse(const QVariant &result)
{
    QJsonRpcMessage response;
    response.d->type = QJsonRpcMessage::Response;
    QJsonObject *object = new QJsonObject;
    object->insert("jsonrpc", QLatin1String("2.0"));
    object->insert("id", d->id);
    object->insert("result", QJsonValue::fromVariant(result));
    response.d->object = object;
    return response;
}

QJsonRpcMessage QJsonRpcMessage::createErrorResponse(QJsonRpc::ErrorCode code, const QString &message, const QVariant &data)
{
    QJsonRpcMessage response;

    QJsonObject error;
    error.insert("code", code);
    if (!message.isEmpty())
        error.insert("message", message);
    if (data.isValid())
        error.insert("data", QJsonValue::fromVariant(data));

    response.d->type = QJsonRpcMessage::Error;
    QJsonObject *object = new QJsonObject;
    object->insert("jsonrpc", QLatin1String("2.0"));
    object->insert("id", d->id);
    object->insert("error", error);
    response.d->object = object;
    return response;
}

int QJsonRpcMessage::id() const
{
    if (d->type == QJsonRpcMessage::Notification)
        return -1;
    return d->object->value("id").toVariant().toInt();
}

QString QJsonRpcMessage::method() const
{
    if (d->type != QJsonRpcMessage::Request)
        return QString();

    return d->object->value("method").toString();
}

QVariantList QJsonRpcMessage::params() const
{
    if (d->type != QJsonRpcMessage::Request)
        return QVariantList();

    return d->object->value("params").toVariant().toList();
}

QVariant QJsonRpcMessage::result() const
{
    if (d->type != QJsonRpcMessage::Response)
        return QVariant();

    return d->object->value("result").toVariant();
}

int QJsonRpcMessage::errorCode() const
{
    if (d->type != QJsonRpcMessage::Error)
        return 0;

    QJsonObject error = d->object->value("error").toObject();
    return error.value("code").toVariant().toInt();
}

QString QJsonRpcMessage::errorMessage() const
{
    if (d->type != QJsonRpcMessage::Error)
        return 0;

    QJsonObject error = d->object->value("error").toObject();
    return error.value("message").toString();
}

QVariant QJsonRpcMessage::errorData() const
{
    if (d->type != QJsonRpcMessage::Error)
        return 0;

    QJsonObject error = d->object->value("error").toObject();
    return error.value("data").toVariant();
}

static QDebug operator<<(QDebug dbg, QJsonRpcMessage::Type type)
{
    switch (type) {
    case QJsonRpcMessage::Request:
        return dbg << "QJsonRpcMessage::Request";
    case QJsonRpcMessage::Response:
        return dbg << "QJsonRpcMessage::Response";
    case QJsonRpcMessage::Notification:
        return dbg << "QJsonRpcMessage::Notification";
    case QJsonRpcMessage::Error:
        return dbg << "QJsonRpcMessage::Error";
    default:
        return dbg << "QJsonRpcMessage::Invalid";
    }
}

QDebug operator<<(QDebug dbg, const QJsonRpcMessage &msg)
{
    dbg.nospace() << "QJsonRpcMessage(type=" << msg.type();
    if (msg.type() != QJsonRpcMessage::Notification) {
        dbg.nospace() << ", id=" << msg.id();
    }

    if (msg.type() == QJsonRpcMessage::Request) {
        dbg.nospace() << ", method=" << msg.method()
                      << ", params=" << msg.params();
    } else if (msg.type() == QJsonRpcMessage::Response) {
        dbg.nospace() << ", result=" << msg.result();
    } else if (msg.type() == QJsonRpcMessage::Error) {
        dbg.nospace() << ", code=" << msg.errorCode()
                      << ", message=" << msg.errorMessage()
                      << ", data=" << msg.errorData();
    }
    dbg.nospace() << ")";
    return dbg.space();
}


QJsonRpcService::QJsonRpcService(QObject *parent)
    : QObject(parent)
{
}

void QJsonRpcService::cacheInvokableInfo()
{
    const QMetaObject *obj = metaObject();
    int startIdx = QObject::staticMetaObject.methodCount(); // skip QObject slots
    for (int idx = startIdx; idx < obj->methodCount(); ++idx) {
        const QMetaMethod method = obj->method(idx);
        if (method.methodType() == QMetaMethod::Slot) {
            QByteArray signature = method.signature();
            m_invokableMethodHash[signature.left(signature.indexOf('('))] = idx;

            QList<int> parameterTypes;
            parameterTypes << QMetaType::type(method.typeName());

            foreach(QByteArray parameterType, method.parameterTypes()) {
                parameterTypes << QMetaType::type(parameterType);
            }
            m_parameterTypeHash[idx] = parameterTypes;
        }
    }
}

QJsonValue QJsonRpcService::dispatch(const QByteArray &method, const QVariantList &arguments) const
{    
    if (!m_invokableMethodHash.contains(method)) {
        /*
        QJsonRpcError error;
        error.setCode(QJsonRpc::MethodNotFound);
        error.setMessage("invalid method called");
        return error.serialize();
        */
        return false;
    }

    int idx = m_invokableMethodHash.value(method);
    const QList<int> parameterTypes = m_parameterTypeHash.value(idx);

    if (arguments.size() != parameterTypes.size() - 1) {
        /*
        QJsonRpcError error;
        error.setCode(QJsonRpc::InvalidParams);
        error.setMessage("invalid parameters");
        return error.serialize();
        */
        return false;
    }

    // QList<QVariant> auxArguments;
    QVarLengthArray<void *, 10> parameters;
    parameters.reserve(parameterTypes.count());

    // first argument to metacall is the return value
    void *null = 0;
    QVariant returnValue(parameterTypes[0], null);
    parameters.append(const_cast<void *>(returnValue.constData()));

    // compile arguments
    for (int i = 0; i < parameterTypes.size() - 1; ++i) {
//        int parameterType = parameterTypes[i + 1];
        const QVariant &argument = arguments.at(i);
//        if (argument.userType() == parameterType) {
            parameters.append(const_cast<void *>(argument.constData()));
//        } else {
//        }
    }

    bool success = false;
    success = const_cast<QJsonRpcService*>(this)->qt_metacall(QMetaObject::InvokeMetaMethod, idx, parameters.data()) < 0;
    if (!success) {
        /*
        QJsonRpcError error;
        error.setCode(QJsonRpc::InvalidParams);
        error.setMessage(QString("dispatch for method '%1' failed").arg(method.constData()));
        return error.serialize();
        */
        return false;
    }

    return QJsonValue::fromVariant(returnValue);
}



QJsonRpcPeer::QJsonRpcPeer(QObject *parent)
    : QObject(parent),
      m_server(0),
      m_client(0)
{
}

QJsonRpcPeer::~QJsonRpcPeer()
{
    // get rid of any client references we might have
    while (!m_clients.isEmpty()) {
        QLocalSocket *client = m_clients.takeFirst();
        client->close();
        client->deleteLater();
    }
}

void QJsonRpcPeer::addService(QJsonRpcService *service)
{
    m_services.insert(service->serviceName(), service);
    service->cacheInvokableInfo();
}

bool QJsonRpcPeer::listenForPeers(const QString &socket)
{
    if (!m_server) {
        m_server = new QLocalServer(this);
        connect(m_server, SIGNAL(newConnection()), this, SLOT(processIncomingConnection()));
    }

    return m_server->listen(socket);
}

void QJsonRpcPeer::connectToPeer(const QString &socket)
{
    if (m_client) {
        m_client->disconnectFromServer();
        m_client->deleteLater();
    }

    m_client = new QLocalSocket(this);
    m_client->connectToServer(socket);
    connect(m_client, SIGNAL(readyRead()), this, SLOT(processIncomingData()));
}

void QJsonRpcPeer::processIncomingConnection()
{
    QLocalSocket *client = m_server->nextPendingConnection();
    connect(client, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
    connect(client, SIGNAL(readyRead()), this, SLOT(processIncomingData()));
    m_clients.append(client);
}

void QJsonRpcPeer::clientDisconnected()
{
    QLocalSocket *client = static_cast<QLocalSocket*>(sender());
    if (!client) {
        return;
    }

    if (m_clients.contains(client)) {
        m_clients.removeAll(client);
    }

    client->deleteLater();
}


void QJsonRpcPeer::sendMessage(QLocalSocket *socket, const QJsonRpcMessage &message)
{
    QJsonDocument doc = QJsonDocument(message.toObject());
    socket->write(doc.toBinaryData());
}

void QJsonRpcPeer::processIncomingData()
{
    QLocalSocket *socket = static_cast<QLocalSocket*>(sender());
    if (!socket) {
        return;
    }

    QByteArray data = socket->readAll();
    while (!data.isEmpty()) {
        QJsonDocument document = QJsonDocument::fromBinaryData(data);
        if (!document.isObject()) {
            qDebug() << "unable to parse message data: " << data;
            return;
        }

        // really inefficient...
        data = data.mid(document.toBinaryData().size());

        // process the message
        QJsonRpcMessage message(document.object());
        if (message.type() == QJsonRpcMessage::Request) {
            QStringList serviceBits = message.method().split(".");
            QString serviceName = serviceBits.takeFirst();
            QString method = serviceBits.takeFirst();
            if (serviceName.isEmpty() || !m_services.contains(serviceName)) {
                QJsonRpcMessage error = message.createErrorResponse(QJsonRpc::MethodNotFound,
                                                                    QString("service '%1' not found").arg(serviceName));
                sendMessage(socket, error);
                return;
            }

            QJsonRpcService *service = m_services.value(serviceName);
            QJsonValue result = service->dispatch(method.toLatin1(), message.params());
            QJsonRpcMessage response = message.createResponse(result.toVariant());
            sendMessage(socket, response);
        } else if (message.type() == QJsonRpcMessage::Response ||
                   message.type() == QJsonRpcMessage::Notification) {
            Q_EMIT messageReceived(message);
        } else {
            qDebug() << "invalid message received: " << message;
        }
    }
}


void QJsonRpcPeer::callRemoteMethod(const QString &method, const QVariant &param1,
                                    const QVariant &param2, const QVariant &param3,
                                    const QVariant &param4, const QVariant &param5,
                                    const QVariant &param6, const QVariant &param7,
                                    const QVariant &param8, const QVariant &param9,
                                    const QVariant &param10)
{
    if (!m_client) {
        qDebug() << "not active connection, aborting.";
        return;
    }

    QVariantList params;
    if (param1.isValid()) params.append(param1);
    if (param2.isValid()) params.append(param2);
    if (param3.isValid()) params.append(param3);
    if (param4.isValid()) params.append(param4);
    if (param5.isValid()) params.append(param5);
    if (param6.isValid()) params.append(param6);
    if (param7.isValid()) params.append(param7);
    if (param8.isValid()) params.append(param8);
    if (param9.isValid()) params.append(param9);
    if (param10.isValid()) params.append(param10);

    QJsonRpcMessage request = QJsonRpcMessage::createRequest(method, params);
    sendMessage(m_client, request);
}





