#ifndef QJSONRPC_P_H
#define QJSONRPC_P_H

#include <QSharedData>

#include "qjsonvalue.h"
#include "qjsonrpc.h"

class QJsonRpcMessagePrivate : public QSharedData
{
public:
    QJsonRpcMessagePrivate();
    ~QJsonRpcMessagePrivate();

    static QJsonRpcMessage createBasicRequest(const QString &method, const QVariantList &params);

    QString id;
    QJsonRpcMessage::Type type;
    QJsonObject *object;

    static int uniqueRequestCounter;

};

#endif
