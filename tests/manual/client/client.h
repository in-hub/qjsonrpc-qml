#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include "qjsonrpc.h"

class Client : public QObject
{
    Q_OBJECT
public:
    Client(QObject *parent = 0);
    void run();

private Q_SLOTS:
    void processMessage(const QJsonRpcMessage &response);

private:
    QJsonRpcPeer *m_peer;

};

#endif
