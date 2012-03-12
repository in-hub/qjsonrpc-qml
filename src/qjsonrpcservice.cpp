#include <QLocalSocket>
#include <QLocalServer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVarLengthArray>
#include <QStringList>
#include <QMetaMethod>

#include "json/qjsondocument.h"
#include "qjsonrpcservice_p.h"
#include "qjsonrpcservice.h"

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

QJsonRpcMessage QJsonRpcService::dispatch(const QJsonRpcMessage &request) const
{
    if (!request.type() == QJsonRpcMessage::Request) {
        QJsonRpcMessage error =
                request.createErrorResponse(QJsonRpc::InvalidRequest, "invalid request");
        return error;
    }

    QByteArray method = request.method().section(".", 1).toLatin1();
    QVariantList arguments = request.params();
    if (!m_invokableMethodHash.contains(method)) {
        QJsonRpcMessage error =
            request.createErrorResponse(QJsonRpc::MethodNotFound, "invalid method called");
        return error;
    }

    int idx = m_invokableMethodHash.value(method);
    const QList<int> parameterTypes = m_parameterTypeHash.value(idx);

    if (arguments.size() != parameterTypes.size() - 1) {
        QJsonRpcMessage error =
            request.createErrorResponse(QJsonRpc::InvalidParams, "invalid parameters");
        return error;
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
        int parameterType = parameterTypes[i + 1];
        const QVariant &argument = arguments.at(i);
        if (argument.userType() != parameterType)
            const_cast<QVariant*>(&argument)->convert(static_cast<QVariant::Type>(parameterType));
        parameters.append(const_cast<void *>(argument.constData()));

    }

    bool success = false;
    success = const_cast<QJsonRpcService*>(this)->qt_metacall(QMetaObject::InvokeMetaMethod, idx, parameters.data()) < 0;
    if (!success) {
        QString message = QString("dispatch for method '%1' failed").arg(method.constData());
        QJsonRpcMessage error =
            request.createErrorResponse(QJsonRpc::InvalidRequest, message);
        return error;
    }

    return request.createResponse(returnValue);
}


QJsonRpcServiceReply::QJsonRpcServiceReply(QObject *parent)
    : QObject(parent)
{
}

QJsonRpcMessage QJsonRpcServiceReply::response() const
{
    return m_response;
}

QJsonRpcServiceSocket::QJsonRpcServiceSocket(QIODevice *device, QObject *parent)
    : QObject(parent),
      d(new QJsonRpcServiceSocketPrivate)
{
    connect(device, SIGNAL(readyRead()), this, SLOT(processIncomingData()));
    d->device = device;
}

QJsonRpcServiceSocket::~QJsonRpcServiceSocket()
{
    if (d->device)
        d->device.data()->deleteLater();
}

bool QJsonRpcServiceSocket::isValid() const
{
    return d->device && d->device.data()->isOpen();
}

/*
void QJsonRpcServiceSocket::sendMessage(const QList<QJsonRpcMessage> &messages)
{
    QJsonArray array;
    foreach (QJsonRpcMessage message, messages) {
        array.append(message.toObject());
    }

    QJsonDocument doc = QJsonDocument(array);
    m_device.data()->write(doc.toBinaryData());
}
*/

QJsonRpcServiceReply *QJsonRpcServiceSocket::sendMessage(const QJsonRpcMessage &message)
{
    QJsonDocument doc = QJsonDocument(message.toObject());
    // m_device.data()->write(doc.toBinaryData());
    d->device.data()->write(doc.toJson());

    QJsonRpcServiceReply *reply = new QJsonRpcServiceReply(this);
    d->replies.insert(message.id(), reply);
    return reply;
}

QJsonRpcServiceReply *QJsonRpcServiceSocket::invokeRemoteMethod(const QString &method, const QVariant &param1,
                                               const QVariant &param2, const QVariant &param3,
                                               const QVariant &param4, const QVariant &param5,
                                               const QVariant &param6, const QVariant &param7,
                                               const QVariant &param8, const QVariant &param9,
                                               const QVariant &param10)
{
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
    return sendMessage(request);
}

void QJsonRpcServiceSocket::processIncomingData()
{
    if (!d->device) {
        qDebug() << Q_FUNC_INFO << "called without device";
        return;
    }

    QByteArray data = d->device.data()->readAll();
    while (!data.isEmpty()) {
        // NOTE: sending this stuff in binary breaks compatibility with
        //       other jsonrpc implementations
        // QJsonDocument document = QJsonDocument::fromBinaryData(data);
        // data = data.mid(document.toBinaryData().size());

        QJsonDocument document = QJsonDocument::fromJson(data);
        data = data.mid(document.toJson().size());
        if (document.isArray()) {
            qDebug() << "bulk support is current disabled";
            /*
            for (int i = 0; i < document.array().size(); ++i) {
                QJsonObject messageObject = document.array().at(i).toObject();
                if (!messageObject.isEmpty()) {
                    QJsonRpcMessage message(messageObject);
                    Q_EMIT messageReceived(message);
                }
            }
            */
        } else if (document.isObject()){
            QJsonRpcMessage message(document.object());
            if (message.type() == QJsonRpcMessage::Response) {
                if (d->replies.contains(message.id())) {
                    QJsonRpcServiceReply *reply = d->replies.take(message.id());
                    reply->m_response = message;
                    reply->finished();
                } else {
                    qDebug() << "invalid response receieved: " << message;
                }
            } else {
                Q_EMIT messageReceived(message);
            }
        }
    }
}

QJsonRpcServiceProvider::QJsonRpcServiceProvider(QTcpServer *server, QObject *parent)
    : QObject(parent),
      d(new QJsonRpcServiceProviderPrivate)
{
    d->type = QJsonRpcServiceProvider::TcpServer;
    connect(server, SIGNAL(newConnection()), this, SLOT(processIncomingConnection()));
    d->tcpServer = server;
}

QJsonRpcServiceProvider::QJsonRpcServiceProvider(QLocalServer *server, QObject *parent)
    : QObject(parent),
      d(new QJsonRpcServiceProviderPrivate)
{
    d->type = QJsonRpcServiceProvider::LocalServer;
    connect(server, SIGNAL(newConnection()), this, SLOT(processIncomingConnection()));
    d->localServer = server;
}

QJsonRpcServiceProvider::~QJsonRpcServiceProvider()
{
    while (!d->clients.isEmpty()) {
        QJsonRpcServiceSocket *client = d->clients.takeFirst();
        client->deleteLater();
    }
}

QJsonRpcServiceProvider::Type QJsonRpcServiceProvider::type() const
{
    return d->type;
}

void QJsonRpcServiceProvider::addService(QJsonRpcService *service)
{
    service->cacheInvokableInfo();
    d->services.insert(service->serviceName(), service);
}

void QJsonRpcServiceProvider::notifyConnectedClients(const QJsonRpcMessage &message)
{
    for (int i = 0; i < d->clients.size(); ++i) {
        d->clients[i]->sendMessage(message);
    }
}

void QJsonRpcServiceProvider::processIncomingConnection()
{
    if (d->type == LocalServer && d->localServer) {
        QLocalSocket *localSocket = d->localServer.data()->nextPendingConnection();
        QIODevice *socket = qobject_cast<QIODevice*>(localSocket);
        QJsonRpcServiceSocket *serviceSocket = new QJsonRpcServiceSocket(socket, this);
        connect(serviceSocket, SIGNAL(messageReceived(QJsonRpcMessage)), this, SLOT(processMessage(QJsonRpcMessage)));
        d->clients.append(serviceSocket);
        connect(localSocket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
        d->localServiceSocketLookup.insert(localSocket, serviceSocket);
    } else if (d->type == TcpServer && d->tcpServer) {
        QTcpSocket *tcpSocket = d->tcpServer.data()->nextPendingConnection();
        QIODevice *socket = qobject_cast<QIODevice*>(tcpSocket);
        QJsonRpcServiceSocket *serviceSocket = new QJsonRpcServiceSocket(socket, this);
        connect(serviceSocket, SIGNAL(messageReceived(QJsonRpcMessage)), this, SLOT(processMessage(QJsonRpcMessage)));
        d->clients.append(serviceSocket);
        connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
        d->tcpServiceSocketLookup.insert(tcpSocket, serviceSocket);
    } else {
        qDebug() << Q_FUNC_INFO << "called without a server";
    }
}

bool QJsonRpcServiceProvider::isListening() const
{
    return (d->localServer && d->localServer.data()->isListening() ||
            d->tcpServer && d->tcpServer.data()->isListening());
}

void QJsonRpcServiceProvider::clientDisconnected()
{
    QLocalSocket *localSocket = static_cast<QLocalSocket*>(sender());
    if (localSocket) {
        if (d->localServiceSocketLookup.contains(localSocket)) {
            QJsonRpcServiceSocket *socket = d->localServiceSocketLookup.take(localSocket);
            d->clients.removeAll(socket);
        }
        localSocket->deleteLater();
        return;
    }

    QTcpSocket *tcpSocket = static_cast<QTcpSocket*>(sender());
    if (tcpSocket) {
        if (d->tcpServiceSocketLookup.contains(tcpSocket)) {
            QJsonRpcServiceSocket *socket = d->tcpServiceSocketLookup.take(tcpSocket);
            d->clients.removeAll(socket);
        }
        tcpSocket->deleteLater();
        return;
    }
}

void QJsonRpcServiceProvider::processMessage(const QJsonRpcMessage &message)
{
    QJsonRpcServiceSocket *serviceSocket = static_cast<QJsonRpcServiceSocket*>(sender());
    if (!serviceSocket) {
        qDebug() << "something went wrong";
        return;
    }

    if (message.type() == QJsonRpcMessage::Request) {
        QString serviceName = message.method().split(".").first();
        if (serviceName.isEmpty() || !d->services.contains(serviceName)) {
            QJsonRpcMessage error = message.createErrorResponse(QJsonRpc::MethodNotFound,
                                                                QString("service '%1' not found").arg(serviceName));
            serviceSocket->sendMessage(error);
            return;
        }

        QJsonRpcService *service = d->services.value(serviceName);
        QJsonRpcMessage response = service->dispatch(message);
        serviceSocket->sendMessage(response);
    } else if (message.type() == QJsonRpcMessage::Response ||
               message.type() == QJsonRpcMessage::Notification) {
        // the server only accepts requests, and notifies connected clients
        // these are unhandled
    } else {
        qDebug() << "invalid message received: " << message;
    }
}
