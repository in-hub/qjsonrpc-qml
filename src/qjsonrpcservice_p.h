#ifndef QJSONRPCSERVICE_P_H
#define QJSONRPCSERVICE_P_H

#include "qjsonrpcservice.h"



class QJsonRpcServiceSocketPrivate
{
public:
    QWeakPointer<QIODevice> device;
    QHash<int, QJsonRpcServiceReply *> replies;

};

class QLocalServer;
class QJsonRpcServiceProviderPrivate
{
public:
    QJsonRpcServiceProviderPrivate() : server(0) {}

    QLocalServer *server;
    QList<QJsonRpcServiceSocket *> clients;
    QHash<QLocalSocket *, QJsonRpcServiceSocket *> serviceSocketLookup;
    QHash<QString, QJsonRpcService *> services;

};

#endif
