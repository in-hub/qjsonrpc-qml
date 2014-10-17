#include <QDebug>

#include "qjsonrpcsocket.h"
#include "qjsonrpcabstractserver_p.h"
#include "qjsonrpcabstractserver.h"

QJsonRpcAbstractServer::QJsonRpcAbstractServer(QJsonRpcAbstractServerPrivate &dd, QObject *parent)
#if defined(USE_QT_PRIVATE_HEADERS)
    : QObject(dd, parent)
#else
    : QObject(parent),
      d_ptr(&dd)
#endif
{
}

QJsonRpcAbstractServer::~QJsonRpcAbstractServer()
{
    Q_D(QJsonRpcAbstractServer);
     foreach (QJsonRpcSocket *client, d->clients)
        client->deleteLater();
    d->clients.clear();
}

bool QJsonRpcAbstractServer::addService(QJsonRpcService *service)
{
    if (!QJsonRpcServiceProvider::addService(service))
        return false;

    connect(service, SIGNAL(notifyConnectedClients(QJsonRpcMessage)),
               this, SLOT(notifyConnectedClients(QJsonRpcMessage)));
    connect(service, SIGNAL(notifyConnectedClients(QString,QJsonArray)),
               this, SLOT(notifyConnectedClients(QString,QJsonArray)));
    return true;
}

bool QJsonRpcAbstractServer::removeService(QJsonRpcService *service)
{
    if (!QJsonRpcServiceProvider::removeService(service))
        return false;

    disconnect(service, SIGNAL(notifyConnectedClients(QJsonRpcMessage)),
                  this, SLOT(notifyConnectedClients(QJsonRpcMessage)));
    disconnect(service, SIGNAL(notifyConnectedClients(QString,QJsonArray)),
                  this, SLOT(notifyConnectedClients(QString,QJsonArray)));
    return true;
}

void QJsonRpcAbstractServer::close()
{
}

int QJsonRpcAbstractServer::connectedClientCount() const
{
    Q_D(const QJsonRpcAbstractServer);
    return d->clients.size();
}

void QJsonRpcAbstractServer::notifyConnectedClients(const QString &method,
                                                    const QJsonArray &params)
{
    QJsonRpcMessage notification =
        QJsonRpcMessage::createNotification(method, params);
    notifyConnectedClients(notification);
}

void QJsonRpcAbstractServer::notifyConnectedClients(const QJsonRpcMessage &message)
{
    Q_D(QJsonRpcAbstractServer);
    for (int i = 0; i < d->clients.size(); ++i)
        d->clients[i]->notify(message);
}

void QJsonRpcAbstractServerPrivate::_q_processMessage(const QJsonRpcMessage &message)
{
    Q_Q(QJsonRpcAbstractServer);
    QJsonRpcSocket *socket = static_cast<QJsonRpcSocket*>(q->sender());
    if (!socket) {
        qJsonRpcDebug() << Q_FUNC_INFO << "called without service socket";
        return;
    }

    q->processMessage(socket, message);
}

#include "moc_qjsonrpcabstractserver.cpp"
