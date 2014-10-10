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
#ifndef QJSONRPCABSTRACTSERVER_P_H
#define QJSONRPCABSTRACTSERVER_P_H

#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#else
#include "json/qjsondocument.h"
#endif

#include "qjsonrpcabstractserver.h"

class QJsonRpcSocket;
#if defined(USE_QT_PRIVATE_HEADERS)
#include <private/qobject_p.h>

class QJsonRpcAbstractServerPrivate : public QObjectPrivate
#else
class QJsonRpcAbstractServerPrivate
#endif
{
public:
#if QT_VERSION >= 0x050100 || QT_VERSION <= 0x050000
    QJsonDocument::JsonFormat format;
    QJsonRpcAbstractServerPrivate(QJsonRpcAbstractServer *server)
        : format(QJsonDocument::Compact),
          q_ptr(server)
    {}
#else
    QJsonRpcAbstractServerPrivate(QJsonRpcAbstractServer *server)
        : q_ptr(server)
    {}
#endif

#if !defined(USE_QT_PRIVATE_HEADERS)
    virtual ~QJsonRpcAbstractServerPrivate() {}
#endif

    virtual void _q_processIncomingConnection() = 0;
    virtual void _q_clientDisconnected() = 0;
    void _q_processMessage(const QJsonRpcMessage &message);

    QList<QJsonRpcSocket*> clients;

    QJsonRpcAbstractServer * const q_ptr;
    Q_DECLARE_PUBLIC(QJsonRpcAbstractServer)
};

#endif
