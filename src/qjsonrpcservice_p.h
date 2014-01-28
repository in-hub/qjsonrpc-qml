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

#include <private/qobject_p.h>

#include <QHash>
#include <QPointer>
#include <QVarLengthArray>
#include <QStringList>

class QJsonRpcSocket;
class QJsonRpcService;
class QJsonRpcServicePrivate : public QObjectPrivate
{
public:
    QJsonRpcServicePrivate(QJsonRpcService *parent)
        : q_ptr(parent)
    {
    }

    void cacheInvokableInfo();
    static int qjsonRpcMessageType;
    QHash<QByteArray, QList<int> > invokableMethodHash;
    QHash<int, QList<int> > parameterTypeHash;    // actual parameter types to convert to
    QHash<int, QList<int> > jsParameterTypeHash;  // for comparing incoming messages
    QHash<int, QStringList> parameterNamesHash;
    QPointer<QJsonRpcSocket> socket;

    QJsonRpcService * const q_ptr;
    Q_DECLARE_PUBLIC(QJsonRpcService)
};

class ObjectCreator
{
public:
    void *create(int type);
    ~ObjectCreator();

private:
    static const int prealloc = 10;
    QVarLengthArray<QPair<void*, int>, prealloc> m_objects;

};

#endif
