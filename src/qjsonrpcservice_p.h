#ifndef QJSONRPCSERVICE_P_H
#define QJSONRPCSERVICE_P_H

#include <QHash>
#include <QHostAddress>
#include <QWeakPointer>
#include <QLocalSocket>
#include <QTcpSocket>

#include "qjsonrpcservice.h"

class QJsonRpcServiceProviderPrivate
{
public:
    QHash<QString, QJsonRpcService*> services;
};

class QJsonRpcSocketPrivate : public QJsonRpcServiceProviderPrivate
{
public:
    QWeakPointer<QIODevice> device;
    QByteArray buffer;
    QHash<int, QJsonRpcServiceReply*> replies;

};

class QJsonRpcServerPrivate : public QJsonRpcServiceProviderPrivate
{
public:
    QList<QJsonRpcSocket*> clients;
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
