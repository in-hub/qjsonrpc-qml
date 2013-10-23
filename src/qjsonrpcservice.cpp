/*
 * Copyright (C) 2012-2013 Matt Broadstone
 * Contact: http://bitbucket.org/devonit/qjsonrpc
 *
 * This file is part of the QJsonRpc Library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */
#include <QLocalSocket>
#include <QLocalServer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVarLengthArray>
#include <QStringList>
#include <QMetaMethod>
#include <QEventLoop>
#include <QTimer>

#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#else
#include "json/qjsondocument.h"
#endif

#include "qjsonrpcservice_p.h"
#include "qjsonrpcservice.h"
#include "qjsonrpcservicereply.h"

int QJsonRpcSocketPrivate::findJsonDocumentEnd(const QByteArray &jsonData)
{
    const char* pos = jsonData.constData();
    const char* end = pos + jsonData.length();

    char blockStart = 0;
    char blockEnd = 0;
    int index = 0;

    // Find the beginning of the JSON document and determine if it is an object or an array
    while (true) {
        if (pos == end) {
            return -1;
        } else if (*pos == '{') {
            blockStart = '{';
            blockEnd = '}';
            break;
        } else if(*pos == '[') {
            blockStart = '[';
            blockEnd = ']';
            break;
        }

        pos++;
        index++;
    }

    // Find the end of the JSON document
    pos++;
    index++;
    int depth = 1;
    bool inString = false;
    while (depth > 0 && pos != end) {
        if (*pos == '\\') {
            pos += 2;
            index += 2;
            continue;
        } else if (*pos == '"') {
            inString = !inString;
        } else if (!inString) {
            if (*pos == blockStart)
                depth++;
            else if (*pos == blockEnd)
                depth--;
        }

        pos++;
        index++;
    }

    // index-1 because we are one position ahead
    return depth == 0 ? index-1 : -1;
}

void QJsonRpcSocketPrivate::writeData(const QJsonRpcMessage &message)
{
    QJsonDocument doc = QJsonDocument(message.toObject());

#if QT_VERSION >= 0x050100 || QT_VERSION <= 0x050000
    QByteArray data = doc.toJson(format);
#else
    QByteArray data = doc.toJson();
#endif

    device.data()->write(data);
    if (qgetenv("QJSONRPC_DEBUG").toInt())
        qDebug() << "sending: " << data;
}

QJsonRpcService::QJsonRpcService(QObject *parent)
    : QObject(parent),
      d_ptr(new QJsonRpcServicePrivate(this))
{
}

QJsonRpcService::~QJsonRpcService()
{
}

QJsonRpcSocket *QJsonRpcService::senderSocket()
{
    Q_D(QJsonRpcService);
    if (d->socket)
        return d->socket.data();
    return 0;
}

int convertVariantTypeToJSType(int type)
{
    switch (type) {
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Double:
    case QMetaType::Long:
    case QMetaType::LongLong:
    case QMetaType::Short:
    case QMetaType::Char:
    case QMetaType::ULong:
    case QMetaType::ULongLong:
    case QMetaType::UShort:
    case QMetaType::UChar:
    case QMetaType::Float:
        return QMetaType::Double;    // all numeric types in js are doubles
        break;

    default:
        break;
    }

    return type;
}

int QJsonRpcServicePrivate::qjsonRpcMessageType = qRegisterMetaType<QJsonRpcMessage>("QJsonRpcMessage");
void QJsonRpcServicePrivate::cacheInvokableInfo()
{
    Q_Q(QJsonRpcService);
    const QMetaObject *obj = q->metaObject();
    int startIdx = q->staticMetaObject.methodCount(); // skip QObject slots
    for (int idx = startIdx; idx < obj->methodCount(); ++idx) {
        const QMetaMethod method = obj->method(idx);
        if ((method.methodType() == QMetaMethod::Slot &&
             method.access() == QMetaMethod::Public) ||
             method.methodType() == QMetaMethod::Signal) {
#if QT_VERSION >= 0x050000
            QByteArray methodName = method.name();
#else
            QByteArray signature = method.signature();
            QByteArray methodName = signature.left(signature.indexOf('('));
#endif
            invokableMethodHash.insert(methodName, idx);

            QList<int> parameterTypes;
            QList<int> jsParameterTypes;
#if QT_VERSION >= 0x050000
            parameterTypes << method.returnType();
#else
            parameterTypes << QMetaType::type(method.typeName());
#endif
            foreach(QByteArray parameterType, method.parameterTypes()) {
                parameterTypes << QMetaType::type(parameterType);
                jsParameterTypes << convertVariantTypeToJSType(QMetaType::type(parameterType));                
            }

            parameterTypeHash[idx] = parameterTypes;
            jsParameterTypeHash[idx] = jsParameterTypes;
        }
    }
}

bool variantAwareCompare(const QList<int> &argumentTypes, const QList<int> &jsParameterTypes)
{
    if (argumentTypes.size() != jsParameterTypes.size())
        return false;

    for (int i = 0; i < argumentTypes.size(); ++i) {
        if (argumentTypes.at(i) == jsParameterTypes.at(i))
            continue;
        else if (jsParameterTypes.at(i) == QMetaType::QVariant)
            continue;
        else if (argumentTypes.at(i) == QMetaType::QVariantList) {
            if (jsParameterTypes.at(i) == QMetaType::QStringList)
                continue;

            if (jsParameterTypes.at(i) == QMetaType::QVariantList)
                continue;

            return false;
        } else
            return false;
    }

    return true;
}

//QJsonRpcMessage QJsonRpcService::dispatch(const QJsonRpcMessage &request) const
bool QJsonRpcService::dispatch(const QJsonRpcMessage &request)
{
    Q_D(QJsonRpcService);
    if (request.type() != QJsonRpcMessage::Request &&
        request.type() != QJsonRpcMessage::Notification) {
        QJsonRpcMessage error =
            request.createErrorResponse(QJsonRpc::InvalidRequest, "invalid request");
        Q_EMIT result(error);
        return false;
    }

    QByteArray method = request.method().section(".", -1).toLatin1();
    if (!d->invokableMethodHash.contains(method)) {
        QJsonRpcMessage error =
            request.createErrorResponse(QJsonRpc::MethodNotFound, "invalid method called");
        Q_EMIT result(error);
        return false;
    }

    int idx = -1;
    QList<int> parameterTypes;
    QList<int> indexes = d->invokableMethodHash.values(method);
    QVariantList arguments = request.params();
    QList<int> argumentTypes;
    foreach (QVariant argument, arguments) {
        argumentTypes.append(static_cast<int>(argument.type()));
    }

    foreach (int methodIndex, indexes) {
        if (variantAwareCompare(argumentTypes, d->jsParameterTypeHash[methodIndex])) {
            parameterTypes = d->parameterTypeHash[methodIndex];
            idx = methodIndex;
            break;
        }
    }

    if (idx == -1) {
        QJsonRpcMessage error =
            request.createErrorResponse(QJsonRpc::InvalidParams, "invalid parameters");
        Q_EMIT result(error);
        return false;
    }

    QVarLengthArray<void *, 10> parameters;
    parameters.reserve(parameterTypes.count());

    // first argument to metacall is the return value
    QMetaType::Type returnType = static_cast<QMetaType::Type>(parameterTypes[0]);
#if QT_VERSION >= 0x050000
    void *returnData = QMetaType::create(returnType);
#else
    void *returnData = QMetaType::construct(returnType);
#endif

    QVariant returnValue(returnType, returnData);
    if (returnType == QMetaType::QVariant)
        parameters.append(&returnValue);
    else
        parameters.append(returnValue.data());

    // compile arguments
    QHash<void*, QMetaType::Type> cleanup;
    for (int i = 0; i < parameterTypes.size() - 1; ++i) {
        int parameterType = parameterTypes[i + 1];
        const QVariant &argument = arguments.at(i);
        if (!argument.isValid()) {
            // pass in a default constructed parameter in this case
#if QT_VERSION >= 0x050000
            void *value = QMetaType::create(parameterType);
#else
            void *value = QMetaType::construct(parameterType);
#endif
            parameters.append(value);
            cleanup.insert(value, static_cast<QMetaType::Type>(parameterType));
        } else {
            if (argument.userType() != parameterType &&
                parameterType != QMetaType::QVariant &&
                const_cast<QVariant*>(&argument)->canConvert(static_cast<QVariant::Type>(parameterType)))
                const_cast<QVariant*>(&argument)->convert(static_cast<QVariant::Type>(parameterType));
            parameters.append(const_cast<void *>(argument.constData()));
        }
    }

    bool success =
        const_cast<QJsonRpcService*>(this)->qt_metacall(QMetaObject::InvokeMetaMethod, idx, parameters.data()) < 0;
    if (!success) {
        QString message = QString("dispatch for method '%1' failed").arg(method.constData());
        QJsonRpcMessage error =
            request.createErrorResponse(QJsonRpc::InvalidRequest, message);
        Q_EMIT result(error);
        return false;
    }

    // cleanup and result
    QVariant returnCopy(returnValue);
    QMetaType::destroy(returnType, returnData);
    foreach (void *value, cleanup.keys()) {
        cleanup.remove(value);
        QMetaType::destroy(cleanup.value(value), value);
    }

    Q_EMIT result(request.createResponse(returnCopy));
    return true;
}

QJsonRpcServiceProvider::QJsonRpcServiceProvider()
    : d_ptr(new QJsonRpcServiceProviderPrivate)
{
}

QJsonRpcServiceProvider::~QJsonRpcServiceProvider()
{
}

QByteArray QJsonRpcServiceProviderPrivate::serviceName(QJsonRpcService *service)
{
    const QMetaObject *mo = service->metaObject();
    for (int i = 0; i < mo->classInfoCount(); i++) {
        const QMetaClassInfo mci = mo->classInfo(i);
        if (mci.name() == QLatin1String("serviceName"))
            return mci.value();
    }

    return QByteArray(mo->className()).toLower();
}

bool QJsonRpcServiceProvider::addService(QJsonRpcService *service)
{
    Q_D(QJsonRpcServiceProvider);
    QByteArray serviceName = d->serviceName(service);
    if (serviceName.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "service added without serviceName classinfo, aborting";
        return false;
    }

    if (d->services.contains(serviceName)) {
        qDebug() << Q_FUNC_INFO << "service with name " << serviceName << " already exist";
        return false;
    }

    service->d_ptr->cacheInvokableInfo();
    d->services.insert(serviceName, service);
    if (!service->parent())
        d->cleanupHandler.add(service);
    return true;
}

bool QJsonRpcServiceProvider::removeService(QJsonRpcService *service)
{
    Q_D(QJsonRpcServiceProvider);
    QByteArray serviceName = d->serviceName(service);
    if (!d->services.contains(serviceName)) {
        qDebug() << Q_FUNC_INFO << "can nof find service with name " << serviceName;
        return false;
    }

    d->cleanupHandler.remove(d->services.value(serviceName));
    d->services.remove(serviceName);
    return true;
}

void QJsonRpcServiceProvider::processMessage(QJsonRpcSocket *socket, const QJsonRpcMessage &message)
{
    Q_D(QJsonRpcServiceProvider);
    switch (message.type()) {
        case QJsonRpcMessage::Request:
        case QJsonRpcMessage::Notification: {
            QByteArray serviceName = message.method().section(".", 0, -2).toLatin1();
            if (serviceName.isEmpty() || !d->services.contains(serviceName)) {
                if (message.type() == QJsonRpcMessage::Request) {
                    QJsonRpcMessage error =
                        message.createErrorResponse(QJsonRpc::MethodNotFound,
                            QString("service '%1' not found").arg(serviceName.constData()));
                    socket->notify(error);
                }
            } else {
                QJsonRpcService *service = d->services.value(serviceName);
                service->d_ptr->socket = socket;
                if (message.type() == QJsonRpcMessage::Request)
                    QObject::connect(service, SIGNAL(result(QJsonRpcMessage)),
                                      socket, SLOT(notify(QJsonRpcMessage)));
                service->dispatch(message);
            }
        }
        break;

        case QJsonRpcMessage::Response:
            // we don't handle responses in the provider
            break;

        default: {
            QJsonRpcMessage error =
                message.createErrorResponse(QJsonRpc::InvalidRequest, QString("invalid request"));
            socket->notify(error);
            break;
        }
    };
}

QJsonRpcSocket::QJsonRpcSocket(QIODevice *device, QObject *parent)
    : QObject(parent),
      d_ptr(new QJsonRpcSocketPrivate)
{
    Q_D(QJsonRpcSocket);
    connect(device, SIGNAL(readyRead()), this, SLOT(processIncomingData()));
    d->device = device;
}

QJsonRpcSocket::~QJsonRpcSocket()
{
}

bool QJsonRpcSocket::isValid() const
{
    Q_D(const QJsonRpcSocket);
    return d->device && d->device.data()->isOpen();
}

/*
void QJsonRpcSocket::sendMessage(const QList<QJsonRpcMessage> &messages)
{
    QJsonArray array;
    foreach (QJsonRpcMessage message, messages) {
        array.append(message.toObject());
    }

    QJsonDocument doc = QJsonDocument(array);
    m_device.data()->write(doc.toBinaryData());
}
*/

QJsonRpcMessage QJsonRpcSocket::sendMessageBlocking(const QJsonRpcMessage &message, int msecs)
{
    Q_D(QJsonRpcSocket);
    QJsonRpcServiceReply *reply = sendMessage(message);
    QScopedPointer<QJsonRpcServiceReply> replyPtr(reply);

    QEventLoop responseLoop;
    connect(reply, SIGNAL(finished()), &responseLoop, SLOT(quit()));
    QTimer::singleShot(msecs, &responseLoop, SLOT(quit()));
    responseLoop.exec();

    if (!reply->response().isValid()) {
        d->replies.remove(message.id());
        return message.createErrorResponse(QJsonRpc::TimeoutError, "request timed out");
    }

    return reply->response();
}

QJsonRpcServiceReply *QJsonRpcSocket::sendMessage(const QJsonRpcMessage &message)
{
    Q_D(QJsonRpcSocket);
    if (!d->device) {
        qDebug() << Q_FUNC_INFO << "trying to send message without device";
        return 0;
    }

    notify(message);
    QPointer<QJsonRpcServiceReply> reply(new QJsonRpcServiceReply);
    d->replies.insert(message.id(), reply);
    return reply;
}

void QJsonRpcSocket::notify(const QJsonRpcMessage &message)
{
    Q_D(QJsonRpcSocket);
    if (!d->device) {
        qDebug() << Q_FUNC_INFO << "trying to send message without device";
        return;
    }

    // disconnect the result message if we need to
    QJsonRpcService *service = qobject_cast<QJsonRpcService*>(sender());
    if (service)
        disconnect(service, SIGNAL(result(QJsonRpcMessage)), this, SLOT(notify(QJsonRpcMessage)));

    d->writeData(message);
}

QJsonRpcMessage QJsonRpcSocket::invokeRemoteMethodBlocking(const QString &method, const QVariant &param1,
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
    return sendMessageBlocking(request);
}

QJsonRpcServiceReply *QJsonRpcSocket::invokeRemoteMethod(const QString &method, const QVariant &param1,
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

#if QT_VERSION >= 0x050100 || QT_VERSION <= 0x050000
QJsonDocument::JsonFormat QJsonRpcSocket::wireFormat() const
{
    Q_D(const QJsonRpcSocket);
    return d->format;
}

void QJsonRpcSocket::setWireFormat(QJsonDocument::JsonFormat format)
{
    Q_D(QJsonRpcSocket);
    d->format = format;
}
#endif

void QJsonRpcSocket::processIncomingData()
{
    Q_D(QJsonRpcSocket);
    if (!d->device) {
        qDebug() << Q_FUNC_INFO << "called without device";
        return;
    }

    d->buffer.append(d->device.data()->readAll());
    while (!d->buffer.isEmpty()) {
        int dataSize = d->findJsonDocumentEnd(d->buffer);
        if (dataSize == -1) {
            // incomplete data, wait for more
            return;
        }

        QJsonDocument document = QJsonDocument::fromJson(d->buffer);
        if (document.isEmpty())
            break;

        d->buffer = d->buffer.mid(dataSize + 1);
        if (document.isArray()) {
            qDebug() << Q_FUNC_INFO << "bulk support is current disabled";
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
            if (qgetenv("QJSONRPC_DEBUG").toInt())
                qDebug() << "received: " << document.toJson();

            QJsonRpcMessage message(document.object());
            Q_EMIT messageReceived(message);

            if (message.type() == QJsonRpcMessage::Response ||
                message.type() == QJsonRpcMessage::Error) {
                if (d->replies.contains(message.id())) {
                    QPointer<QJsonRpcServiceReply> reply = d->replies.take(message.id());
                    if (!reply.isNull()) {
                        reply->m_response = message;
                        reply->finished();
                    }
                }
            } else {
                processRequestMessage(message);
            }
        }
    }
}

void QJsonRpcSocket::processRequestMessage(const QJsonRpcMessage &message)
{
    Q_UNUSED(message)
    // we don't do anything the default case with requests and notifications,
    // these are only handled by the provider
}

QJsonRpcServiceSocket::QJsonRpcServiceSocket(QIODevice *device, QObject *parent)
    : QJsonRpcSocket(device, parent)
{
}

QJsonRpcServiceSocket::~QJsonRpcServiceSocket()
{
}

void QJsonRpcServiceSocket::processRequestMessage(const QJsonRpcMessage &message)
{
    QJsonRpcServiceProvider::processMessage(this, message);
}

QJsonRpcServer::QJsonRpcServer(QJsonRpcServerPrivate *dd, QObject *parent)
    : QObject(parent),
      d_ptr(dd)
{
}

QJsonRpcServer::~QJsonRpcServer()
{
    Q_D(QJsonRpcServer);
     foreach (QJsonRpcSocket *client, d->clients)
        client->deleteLater();
    d->clients.clear();
}

bool QJsonRpcServer::addService(QJsonRpcService *service)
{
    if (!QJsonRpcServiceProvider::addService(service))
        return false;

    connect(service, SIGNAL(notifyConnectedClients(QJsonRpcMessage)),
               this, SLOT(notifyConnectedClients(QJsonRpcMessage)));
    connect(service, SIGNAL(notifyConnectedClients(QString,QVariantList)),
               this, SLOT(notifyConnectedClients(QString,QVariantList)));
    return true;
}

bool QJsonRpcServer::removeService(QJsonRpcService *service)
{
    if (!QJsonRpcServiceProvider::removeService(service))
        return false;

    disconnect(service, SIGNAL(notifyConnectedClients(QJsonRpcMessage)),
               this, SLOT(notifyConnectedClients(QJsonRpcMessage)));
    disconnect(service, SIGNAL(notifyConnectedClients(QString,QVariantList)),
               this, SLOT(notifyConnectedClients(QString,QVariantList)));
    return true;
}

#if QT_VERSION >= 0x050100 || QT_VERSION <= 0x050000
QJsonDocument::JsonFormat QJsonRpcServer::wireFormat() const
{
    Q_D(const QJsonRpcServer);
    return d->format;
}

void QJsonRpcServer::setWireFormat(QJsonDocument::JsonFormat format)
{
    Q_D(QJsonRpcServer);
    d->format = format;
}
#endif

void QJsonRpcServer::notifyConnectedClients(const QString &method, const QVariantList &params)
{
    QJsonRpcMessage notification = QJsonRpcMessage::createNotification(method, params);
    notifyConnectedClients(notification);
}

void QJsonRpcServer::notifyConnectedClients(const QJsonRpcMessage &message)
{
    Q_D(QJsonRpcServer);
    for (int i = 0; i < d->clients.size(); ++i)
        d->clients[i]->notify(message);
}

void QJsonRpcServer::processMessage(const QJsonRpcMessage &message)
{
    QJsonRpcSocket *socket = static_cast<QJsonRpcSocket*>(sender());
    if (!socket) {
        qDebug() << Q_FUNC_INFO << "called without service socket";
        return;
    }

    QJsonRpcServiceProvider::processMessage(socket, message);
}


//
// LOCAL
//
QJsonRpcLocalServer::QJsonRpcLocalServer(QObject *parent)
    : QJsonRpcServer(new QJsonRpcLocalServerPrivate, parent)
{
}

QJsonRpcLocalServer::~QJsonRpcLocalServer()
{
    Q_D(QJsonRpcLocalServer);
    foreach (QLocalSocket *socket, d->socketLookup.keys())
        socket->deleteLater();
    d->socketLookup.clear();
}

bool QJsonRpcLocalServer::listen(const QString &service)
{
    Q_D(QJsonRpcLocalServer);
    if (!d->server) {
        d->server = new QLocalServer(this);
        connect(d->server, SIGNAL(newConnection()), this, SLOT(processIncomingConnection()));
    }

    return d->server->listen(service);
}

void QJsonRpcLocalServer::processIncomingConnection()
{
    Q_D(QJsonRpcLocalServer);
    QLocalSocket *localSocket = d->server->nextPendingConnection();
    if (!localSocket) {
        qDebug() << Q_FUNC_INFO << "nextPendingConnection is null";
        return;
    }

    QIODevice *device = qobject_cast<QIODevice*>(localSocket);
    QJsonRpcSocket *socket = new QJsonRpcSocket(device, this);
#if QT_VERSION >= 0x050100 || QT_VERSION <= 0x050000
    socket->setWireFormat(d->format);
#endif

    connect(socket, SIGNAL(messageReceived(QJsonRpcMessage)), this, SLOT(processMessage(QJsonRpcMessage)));
    d->clients.append(socket);
    connect(localSocket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
    d->socketLookup.insert(localSocket, socket);
}

void QJsonRpcLocalServer::clientDisconnected()
{
    Q_D(QJsonRpcLocalServer);
    QLocalSocket *localSocket = static_cast<QLocalSocket*>(sender());
    if (localSocket) {
        if (d->socketLookup.contains(localSocket)) {
            QJsonRpcSocket *socket = d->socketLookup.take(localSocket);
            d->clients.removeAll(socket);
            socket->deleteLater();
        }

        localSocket->deleteLater();
    }
}

QString QJsonRpcLocalServer::errorString() const
{
    Q_D(const QJsonRpcLocalServer);
    return d->server->errorString();
}

//
// TCP
//
QJsonRpcTcpServer::QJsonRpcTcpServer(QObject *parent)
    : QJsonRpcServer(new QJsonRpcTcpServerPrivate, parent)
{
}

QJsonRpcTcpServer::~QJsonRpcTcpServer()
{
    Q_D(QJsonRpcTcpServer);
    foreach (QTcpSocket *socket, d->socketLookup.keys())
        socket->deleteLater();
    d->socketLookup.clear();
}

bool QJsonRpcTcpServer::listen(const QHostAddress &address, quint16 port)
{
    Q_D(QJsonRpcTcpServer);
    if (!d->server) {
        d->server = new QTcpServer(this);
        connect(d->server, SIGNAL(newConnection()), this, SLOT(processIncomingConnection()));
    }

    return d->server->listen(address, port);
}

void QJsonRpcTcpServer::processIncomingConnection()
{
    Q_D(QJsonRpcTcpServer);
    QTcpSocket *tcpSocket = d->server->nextPendingConnection();
    if (!tcpSocket) {
        qDebug() << Q_FUNC_INFO << "nextPendingConnection is null";
        return;
    }

    QIODevice *device = qobject_cast<QIODevice*>(tcpSocket);
    QJsonRpcSocket *socket = new QJsonRpcSocket(device, this);
#if QT_VERSION >= 0x050100 || QT_VERSION <= 0x050000
    socket->setWireFormat(d->format);
#endif

    connect(socket, SIGNAL(messageReceived(QJsonRpcMessage)), this, SLOT(processMessage(QJsonRpcMessage)));
    d->clients.append(socket);
    connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
    d->socketLookup.insert(tcpSocket, socket);
}

void QJsonRpcTcpServer::clientDisconnected()
{
    Q_D(QJsonRpcTcpServer);
    QTcpSocket *tcpSocket = static_cast<QTcpSocket*>(sender());
    if (tcpSocket) {
        if (d->socketLookup.contains(tcpSocket)) {
            QJsonRpcSocket *socket = d->socketLookup.take(tcpSocket);
            d->clients.removeAll(socket);
            socket->deleteLater();
        }

        tcpSocket->deleteLater();
    }
}

QString QJsonRpcTcpServer::errorString() const
{
    Q_D(const QJsonRpcTcpServer);
    return d->server->errorString();
}
