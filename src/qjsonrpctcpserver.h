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
#ifndef QJSONRPCTCPSERVER_H
#define QJSONRPCTCPSERVER_H

#include <QHostAddress>

#include "qjsonrpcabstractserver.h"

class QJsonRpcTcpServerPrivate;
class QJSONRPC_EXPORT QJsonRpcTcpServer : public QJsonRpcAbstractServer
{
    Q_OBJECT
public:
    explicit QJsonRpcTcpServer(QObject *parent = 0);
    ~QJsonRpcTcpServer();

    QString errorString() const;
    virtual bool listen(const QHostAddress &address, quint16 port);
    virtual void close();

private:
    Q_DECLARE_PRIVATE(QJsonRpcTcpServer)
    Q_DISABLE_COPY(QJsonRpcTcpServer)
    Q_PRIVATE_SLOT(d_func(), void _q_processIncomingConnection())
    Q_PRIVATE_SLOT(d_func(), void _q_clientDisconnected())

};

#endif
