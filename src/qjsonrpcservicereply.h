#ifndef QJSONRPCSERVICEREPLY_H
#define QJSONRPCSERVICEREPLY_H

#include <QObject>

#include "qjsonrpc_export.h"
#include "qjsonrpcmessage.h"

class QJSONRPC_EXPORT QJsonRpcServiceReply : public QObject
{
    Q_OBJECT
public:
    explicit QJsonRpcServiceReply(QObject *parent = 0);
    QJsonRpcMessage response() const;

Q_SIGNALS:
    void finished();

private:
    QJsonRpcMessage m_response;
    friend class QJsonRpcSocket;
};


#endif // QJSONRPCSERVICEREPLY_H
