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
};

class QJsonRpcServerSocketPrivate : public QJsonRpcServiceProviderPrivate
{
public:
    QWeakPointer<QIODevice> device;
    QByteArray buffer;
    QHash<int, QJsonRpcServiceReply*> replies;

};

class QJsonRpcServerPrivate : public QJsonRpcServiceProviderPrivate
{
public:
    QList<QJsonRpcServerSocket*> clients;
};

class QLocalServer;
class QJsonRpcLocalServerPrivate : public QJsonRpcServerPrivate
{
public:
    QJsonRpcLocalServerPrivate() : server(0) {}
    QLocalServer *server;
    QHash<QLocalSocket*, QJsonRpcServerSocket*> socketLookup;
};

class QTcpServer;
class QJsonRpcTcpServerPrivate : public QJsonRpcServerPrivate
{
public:
    QJsonRpcTcpServerPrivate() : server(0) {}
    QTcpServer *server;
    QHash<QTcpSocket*, QJsonRpcServerSocket*> socketLookup;
};

#endif
