#ifndef QJSONRPCSERVICE_P_H
#define QJSONRPCSERVICE_P_H

#include <QHash>
#include <QHostAddress>
#include <QWeakPointer>
#include <QLocalSocket>
#include <QTcpSocket>

#include "qjsonrpcservice.h"

class QJsonRpcServiceSocketPrivate
{
public:
    QWeakPointer<QIODevice> device;
    QHash<int, QJsonRpcServiceReply *> replies;

};

class QJsonRpcServiceProviderPrivate
{
public:
    QJsonRpcServiceProvider::Type type;
    QWeakPointer<QTcpServer> tcpServer;
    QWeakPointer<QLocalServer> localServer;
    QHash<QLocalSocket *, QJsonRpcServiceSocket *> localServiceSocketLookup;
    QHash<QTcpSocket *, QJsonRpcServiceSocket *> tcpServiceSocketLookup;

    QList<QJsonRpcServiceSocket *> clients;
    QHash<QString, QJsonRpcService *> services;

};

#endif
