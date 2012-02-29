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
    void responseReceived(const QJsonRpcResponse &response);
    void notificationReceived(const QJsonRpcNotification &notification);

private:
    QJsonRpcPeer *m_peer;

};

#endif
