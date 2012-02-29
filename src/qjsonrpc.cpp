#include <QMetaObject>
#include <QMetaProperty>
#include <QMetaMethod>
#include <QStringList>

#include "qjsondocument.h"
#include "qjsonrpc.h"

QJsonRpcMessage::QJsonRpcMessage()
    : m_id(-1)
{
}

QString QJsonRpcMessage::jsonrpc() const
{
    return "2.0";
}

QString QJsonRpcMessage::id() const
{
    return m_id;
}

void QJsonRpcMessage::setId(const QString &id)
{
    m_id = id;
}

QJsonObject QJsonRpcMessage::serialize() const
{
    QJsonObject message;
    message.insert("jsonrpc", QLatin1String("2.0"));
    message.insert("id", m_id);
    return message;
}

bool QJsonRpcMessage::parse(const QJsonObject &data)
{
    if (!data.contains("id") && !data.contains("jsonrpc")) {
        return false;
    }

    if (data.value("jsonrpc").toString() != "2.0") {
        qDebug()  << "invalid JSONRPC 2.0 message";
        return false;
    }

    m_id = data.value("id").toString();
    return true;
}

int QJsonRpcRequest::uniqueRequestCounter = 0;
QJsonRpcRequest::QJsonRpcRequest()
{
    uniqueRequestCounter++;
    setId(QString::number(uniqueRequestCounter));
}

QString QJsonRpcRequest::method() const
{
    return m_method;
}

void QJsonRpcRequest::setMethod(const QString &method)
{
    m_method = method;
}

QJsonArray QJsonRpcRequest::params() const
{
    return m_params;
}

void QJsonRpcRequest::setParams(const QJsonArray &params)
{
    m_params = params;
}

QJsonObject QJsonRpcRequest::serialize() const
{
    QJsonObject request(QJsonRpcMessage::serialize());
    request.insert("method", m_method);
    request.insert("params", m_params);
    return request;
}

bool QJsonRpcRequest::isRequest(const QJsonObject &data)
{
    if (data.contains("method") && data.contains("params"))
        return true;
    return false;
}

bool QJsonRpcRequest::parse(const QJsonObject &data)
{
    if (!QJsonRpcMessage::parse(data))
        return false;

    if (!QJsonRpcRequest::isRequest(data))
        return false;

    m_method = data.value("method").toString();
    m_params = data.value("params").toArray();
    return true;
}

QJsonRpcNotification::QJsonRpcNotification()
{
}

QJsonObject QJsonRpcNotification::serialize() const
{
    QJsonObject data(QJsonRpcRequest::serialize());
    data.remove("id");
    return data;
}

bool QJsonRpcNotification::isNotification(const QJsonObject &data)
{
    if (!data.contains("method") && !data.contains("params"))
        return false;

    if (data.contains("id"))
        return false;

    return true;
}

bool QJsonRpcNotification::parse(const QJsonObject &data)
{
    if (!QJsonRpcRequest::parse(data))
        return false;

    setId(QString());
    return true;
}

QJsonRpcError::QJsonRpcError()
    : m_code(0)
{
}

int QJsonRpcError::code() const
{
    return m_code;
}

void QJsonRpcError::setCode(int code)
{
    m_code = code;
}

QString QJsonRpcError::message() const
{
    return m_message;
}

void QJsonRpcError::setMessage(const QString &message)
{
    m_message = message;
}

QJsonValue QJsonRpcError::data() const
{
    return m_data;
}

void QJsonRpcError::setData(const QJsonValue &data)
{
    m_data = data;
}

QJsonObject QJsonRpcError::serialize() const
{
    QJsonObject error;
    error.insert("code", m_code);
    error.insert("message", m_message);
    if (!m_data.isUndefined())
        error.insert("data", m_data);

    return error;
}

bool QJsonRpcError::isError(const QJsonObject &data)
{
    if (!data.contains("code") || !data.contains("message"))
        return false;
    return true;
}

bool QJsonRpcError::parse(const QJsonObject &data)
{
    if (!QJsonRpcError::isError(data))
        return false;

    m_code = data.value("code").toVariant().toInt();
    m_message = data.value("message").toString();
    if (data.contains("data"))
        m_data = data.value("data");

    return true;
}



QJsonRpcResponse::QJsonRpcResponse()
{
}

QJsonValue QJsonRpcResponse::result() const
{
    return m_result;
}

void QJsonRpcResponse::setResult(const QJsonValue &result)
{
    m_result = result;
}

QJsonRpcError QJsonRpcResponse::error() const
{
    return m_error;
}

void QJsonRpcResponse::setError(const QJsonRpcError &error)
{
    m_error = error;
}

bool QJsonRpcResponse::isError() const
{
    return m_result.isNull();
}

QJsonObject QJsonRpcResponse::serialize() const
{
    QJsonObject response(QJsonRpcMessage::serialize());
    if (isError())
        response.insert("error", m_error.serialize());
    else
        response.insert("result", m_result);
    return response;
}

bool QJsonRpcResponse::isResponse(const QJsonObject &data)
{
    if (data.contains("result") || data.contains("error"))
        return true;
    return false;
}

bool QJsonRpcResponse::parse(const QJsonObject &data)
{
    if (!QJsonRpcMessage::parse(data))
        return false;

    if (!QJsonRpcResponse::isResponse(data))
        return false;

    if (data.contains("result")) {
        m_result = data.value("result");
    } else {
        QJsonObject error = data.value("error").toObject();
        if (!m_error.parse(error)) {
            qDebug() << "unable to parse error data: " << error;
            return false;
        }
    }

    return true;
}










QJsonRpcService::QJsonRpcService(QObject *parent)
    : QObject(parent)
{
}

void QJsonRpcService::buildInvokableHash()
{
    const QMetaObject *obj = metaObject();
    int startIdx = QObject::staticMetaObject.methodCount(); // skip QObject slots
    for (int idx = startIdx; idx < obj->methodCount(); ++idx) {
        if (obj->method(idx).methodType() == QMetaMethod::Slot) {
            QByteArray signature = obj->method(idx).signature();
            m_invokableMethodHash[signature.left(signature.indexOf('('))] = idx;
        }
    }
}

QJsonValue QJsonRpcService::dispatch(const QByteArray &method, const QJsonArray &arguments)
{    
    if (!m_invokableMethodHash.contains(method)) {
        QJsonRpcError error;
        error.setCode(QJsonRpc::MethodNotFound);
        error.setMessage("invalid method called");
        return error.serialize();
    }

    int idx = m_invokableMethodHash.value(method);
    QMetaMethod invokeMethod = metaObject()->method(idx);

    if (arguments.size() > invokeMethod.parameterTypes().size()) {
        QJsonRpcError error;
        error.setCode(QJsonRpc::InvalidParams);
        error.setMessage("invalid parameters");
        return error.serialize();
    }

    // compile generic arguments
    QList<QGenericArgument> genericArguments;
    for (int i = 0; i < arguments.size(); i++) {
        if (invokeMethod.parameterTypes().at(i) == "QVariant") {
            genericArguments << Q_ARG(QVariant, arguments.at(i).toVariant());
        } else {
            // NOTE: hmm I bet I can make this crash...
            //       the problem is that QJson converts all ints into qulonglongs,
            //       so a better way to do this is to check the type and make the
            //       correct conversions. I am being lazy here...
            genericArguments << QGenericArgument(invokeMethod.parameterTypes().at(i), arguments.at(i).toVariant().data());
        }
    }

    // determine return type
    int invokeMethodReturnType = QMetaType::type(invokeMethod.typeName());
    void *invokeMethodReturnValue = QMetaType::construct(invokeMethodReturnType, 0);
    QGenericReturnArgument returnValue(invokeMethod.typeName(), invokeMethodReturnValue);

    if (QMetaObject::invokeMethod(this, method.constData(), returnValue,
                                  genericArguments.value(0, QGenericArgument()),
                                  genericArguments.value(1, QGenericArgument()),
                                  genericArguments.value(2, QGenericArgument()),
                                  genericArguments.value(3, QGenericArgument()),
                                  genericArguments.value(4, QGenericArgument()),
                                  genericArguments.value(5, QGenericArgument()),
                                  genericArguments.value(6, QGenericArgument()),
                                  genericArguments.value(7, QGenericArgument()),
                                  genericArguments.value(8, QGenericArgument()),
                                  genericArguments.value(9, QGenericArgument())))
    {
        if (invokeMethodReturnType == QMetaType::QVariant) {
            return QJsonValue::fromVariant(*(QVariant*)invokeMethodReturnValue);
        } else {
            QVariant returnValue(invokeMethodReturnType, invokeMethodReturnValue);
            QMetaType::destroy(invokeMethodReturnType, invokeMethodReturnValue);
            return QJsonValue::fromVariant(returnValue);
        }
    }

    // something else went wrong!
    QJsonRpcError error;
    error.setCode(QJsonRpc::InvalidParams);
    error.setMessage(QString("dispatch for method '%1' failed").arg(method.constData()));
    return error.serialize();
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
    service->buildInvokableHash();
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

void QJsonRpcPeer::processIncomingData()
{
    QLocalSocket *socket = static_cast<QLocalSocket*>(sender());
    if (!socket) {
        return;
    }

    // QStringList messages = QString(socket->readAll()).split(QRegExp("[ \r\n][ \r\n]*"));
    // NOTE: I think this is the worst way to parse this information, but it works for now.
    //       Ideally, in the future, we can rewok the parser to operate on a streaming basis

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
        QJsonObject message = document.object();
        if (QJsonRpcRequest::isRequest(message)) {
            QJsonRpcRequest request;
            if (request.parse(message)) {
                QStringList serviceBits = request.method().split(".");
                QString serviceName = serviceBits.takeFirst();
                QString method = serviceBits.takeFirst();
                if (serviceName.isEmpty() || !m_services.contains(serviceName)) {
                    QJsonRpcError error;
                    error.setCode(QJsonRpc::MethodNotFound);
                    error.setMessage(QString("service '%1' not found").arg(serviceName));

                    QJsonRpcResponse response;
                    response.setId(request.id());
                    response.setError(error);

                    QJsonDocument doc = QJsonDocument(response.serialize());
                    QByteArray bytes = doc.toBinaryData();
                    socket->write(bytes);
                    return;
                }

                QJsonRpcService *service = m_services.value(serviceName);
                QJsonValue result = service->dispatch(method.toLatin1(), request.params());

                QJsonRpcResponse response;
                response.setId(request.id());
                if (result.isObject()) {
                    QJsonRpcError error;
                    error.parse(result.toObject());
                    response.setError(error);
                } else {
                    response.setResult(result);
                }

                QJsonDocument doc = QJsonDocument(response.serialize());
                QByteArray bytes = doc.toBinaryData();
                socket->write(bytes);
            } else {
                qDebug() << "could not parse request message: " << message;
            }
        } else if (QJsonRpcResponse::isResponse(message)) {
            QJsonRpcResponse response;
            if (response.parse(message)) {
                Q_EMIT responseReceived(response);
            } else {
                qDebug() << "could not parse response message: " << message;
            }
        } else if (QJsonRpcNotification::isNotification(message)) {
            QJsonRpcNotification notification;
            if (notification.parse(message)) {
                Q_EMIT notificationReceived(notification);
            } else {
                qDebug() << "could not parse notification message: " << message;
            }
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

    QJsonArray params;
    if (param1.isValid()) params.append(QJsonValue::fromVariant(param1));
    if (param2.isValid()) params.append(QJsonValue::fromVariant(param2));
    if (param3.isValid()) params.append(QJsonValue::fromVariant(param3));
    if (param4.isValid()) params.append(QJsonValue::fromVariant(param4));
    if (param5.isValid()) params.append(QJsonValue::fromVariant(param5));
    if (param6.isValid()) params.append(QJsonValue::fromVariant(param6));
    if (param7.isValid()) params.append(QJsonValue::fromVariant(param7));
    if (param8.isValid()) params.append(QJsonValue::fromVariant(param8));
    if (param9.isValid()) params.append(QJsonValue::fromVariant(param9));
    if (param10.isValid()) params.append(QJsonValue::fromVariant(param10));


    QJsonRpcRequest request;
    request.setMethod(method);
    request.setParams(params);

    QJsonDocument doc = QJsonDocument(request.serialize());
    QByteArray bytes = doc.toBinaryData();
    m_client->write(bytes);
}





