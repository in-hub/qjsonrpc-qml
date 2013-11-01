#ifndef QJSONRPCABSTRACTSERVER_P_H
#define QJSONRPCABSTRACTSERVER_P_H

#include <QObjectCleanupHandler>

#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#else
#include "json/qjsondocument.h"
#endif

class QJsonRpcService;
class QJsonRpcServiceProviderPrivate
{
public:
    QByteArray serviceName(QJsonRpcService *service);

    QHash<QByteArray, QJsonRpcService*> services;
    QObjectCleanupHandler cleanupHandler;

};

class QJsonRpcSocket;
class QJsonRpcAbstractServerPrivate
{
public:
#if QT_VERSION >= 0x050100 || QT_VERSION <= 0x050000
    QJsonDocument::JsonFormat format;
    QJsonRpcAbstractServerPrivate() : format(QJsonDocument::Compact) {}
#else
    QJsonRpcAbstractServerPrivate() {}
#endif

    QList<QJsonRpcSocket*> clients;

};

#endif
