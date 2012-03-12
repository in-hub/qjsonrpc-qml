#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include "qjsonrpcmessage.h"

class QJsonRpcServiceSocket;
class LocalClient : public QObject
{
    Q_OBJECT
public:
    LocalClient(QObject *parent = 0);
    void run();

private Q_SLOTS:
    void processResponse();

private:
    QJsonRpcServiceSocket *m_client;

};

#endif
