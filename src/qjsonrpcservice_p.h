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
    QList<QJsonRpcServiceSocket *> clients;
    QHash<QString, QJsonRpcService *> services;
};

class QLocalServer;
class QJsonRpcLocalServiceProviderPrivate : public QJsonRpcServiceProviderPrivate
{
public:
    QJsonRpcLocalServiceProviderPrivate() : server(0) {}
    QLocalServer *server;
    QHash<QLocalSocket *, QJsonRpcServiceSocket *> serviceSocketLookup;
};

class QTcpServer;
class QJsonRpcTcpServiceProviderPrivate : public QJsonRpcServiceProviderPrivate
{
public:
    QJsonRpcTcpServiceProviderPrivate() : server(0) {}
    QTcpServer *server;
    QHash<QTcpSocket *, QJsonRpcServiceSocket *> serviceSocketLookup;
};

#endif
