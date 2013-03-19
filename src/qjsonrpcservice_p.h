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
#ifndef QJSONRPCSERVICE_P_H
#define QJSONRPCSERVICE_P_H

#include <QHash>
#include <QHostAddress>
#include <QPointer>
#include <QLocalSocket>
#include <QTcpSocket>
#include <QObjectCleanupHandler>

#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#else
#include "qjsondocument.h"
#endif

#include "qjsonrpcservice.h"
#include "qjsonrpcmessage.h"
#include "qjsonrpc_export.h"

class QJsonRpcSocket;
class QJsonRpcService;
class QJsonRpcServicePrivate
{
public:
    QJsonRpcServicePrivate(QJsonRpcService *parent)
        : q_ptr(parent)
    {
    }

    void cacheInvokableInfo();
    static int qjsonRpcMessageType;
    QMultiHash<QByteArray, int> invokableMethodHash;
    QHash<int, QList<int> > parameterTypeHash;    // actual parameter types to convert to
    QHash<int, QList<int> > jsParameterTypeHash;  // for comparing incoming messages
    QPointer<QJsonRpcSocket> socket;

    QJsonRpcService * const q_ptr;
    Q_DECLARE_PUBLIC(QJsonRpcService)
};

class QJsonRpcServiceProviderPrivate
{
public:
    QHash<QString, QJsonRpcService*> services;
    QObjectCleanupHandler cleanupHandler;

};

class QJsonRpcServiceReply;
class QJSONRPC_EXPORT QJsonRpcSocketPrivate
{
public:
    QJsonRpcSocketPrivate() : format(QJsonDocument::Compact) {}
    int findJsonDocumentEnd(const QByteArray &jsonData);
    void writeData(const QJsonRpcMessage &message);

    QPointer<QIODevice> device;
    QByteArray buffer;
    QHash<int, QJsonRpcServiceReply*> replies;
    QJsonDocument::JsonFormat format;

};

class QJsonRpcServerPrivate
{
public:
    QJsonRpcServerPrivate() : format(QJsonDocument::Compact) {}
    QList<QJsonRpcSocket*> clients;
    QJsonDocument::JsonFormat format;
};

class QLocalServer;
class QJsonRpcLocalServerPrivate : public QJsonRpcServerPrivate
{
public:
    QJsonRpcLocalServerPrivate() : server(0) {}
    QLocalServer *server;
    QHash<QLocalSocket*, QJsonRpcSocket*> socketLookup;
};

class QTcpServer;
class QJsonRpcTcpServerPrivate : public QJsonRpcServerPrivate
{
public:
    QJsonRpcTcpServerPrivate() : server(0) {}
    QTcpServer *server;
    QHash<QTcpSocket*, QJsonRpcSocket*> socketLookup;
};

#endif
