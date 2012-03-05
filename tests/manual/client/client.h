#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include "qjsonrpcmessage.h"

class QJsonRpcServiceSocket;
class Client : public QObject
{
    Q_OBJECT
public:
    Client(QObject *parent = 0);
    void run();

private Q_SLOTS:
    void clientConnected();
    void processResponse();

private:
    QJsonRpcServiceSocket *m_client;

};

#endif
