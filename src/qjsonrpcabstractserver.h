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
#ifndef QJSONRPCABSTRACTSERVER_H
#define QJSONRPCABSTRACTSERVER_H

#include <QObject>
#include <QScopedPointer>

#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#else
#include "json/qjsondocument.h"
#endif

#include "qjsonrpcmessage.h"
#include "qjsonrpcglobal.h"

class QJsonRpcService;
class QJsonRpcAbstractSocket;
class QJsonRpcServiceProviderPrivate;
class QJSONRPC_EXPORT QJsonRpcServiceProvider
{
public:
    ~QJsonRpcServiceProvider();
    virtual bool addService(QJsonRpcService *service);
    virtual bool removeService(QJsonRpcService *service);

protected:
    QJsonRpcServiceProvider();
    void processMessage(QJsonRpcAbstractSocket *socket, const QJsonRpcMessage &message);

private:
    QScopedPointer<QJsonRpcServiceProviderPrivate> d;

};

class QJsonRpcAbstractServerPrivate;
class QJSONRPC_EXPORT QJsonRpcAbstractServer : public QObject, public QJsonRpcServiceProvider
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

    virtual void close();
    int connectedClientCount() const;

Q_SIGNALS:
    void clientConnected();
    void clientDisconnected();

public Q_SLOTS:
    void notifyConnectedClients(const QJsonRpcMessage &message);
    void notifyConnectedClients(const QString &method, const QJsonArray &params);

protected:
    explicit QJsonRpcAbstractServer(QJsonRpcAbstractServerPrivate &dd, QObject *parent);

    Q_DECLARE_PRIVATE(QJsonRpcAbstractServer)
    Q_DISABLE_COPY(QJsonRpcAbstractServer)
    Q_PRIVATE_SLOT(d_func(), void _q_processMessage(const QJsonRpcMessage &message))

#if !defined(USE_QT_PRIVATE_HEADERS)
    QScopedPointer<QJsonRpcAbstractServerPrivate> d_ptr;
#endif
};

#endif
