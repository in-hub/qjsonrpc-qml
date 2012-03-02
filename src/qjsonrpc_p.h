#ifndef QJSONRPC_P_H
#define QJSONRPC_P_H

#include <QSharedData>
#include "qjsonvalue.h"

class QJsonRpcMessagePrivate : public QSharedData
{
public:
    QJsonRpcMessagePrivate();
    ~QJsonRpcMessagePrivate();

    QString id;
    int type;
    QJsonObject *object;

    static int uniqueRequestCounter;

};

#endif
