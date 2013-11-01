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

#include "qjsonrpcabstractserver_p.h"
#include "qjsonrpcsocket.h"
#include "qjsonrpcservicereply.h"
#include "qjsonrpcservice_p.h"
#include "qjsonrpcservice.h"

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

//
// LOCAL
//
QJsonRpcLocalServer::QJsonRpcLocalServer(QObject *parent)
    : QJsonRpcAbstractServer(new QJsonRpcLocalServerPrivate, parent)
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
    : QJsonRpcAbstractServer(new QJsonRpcTcpServerPrivate, parent)
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
