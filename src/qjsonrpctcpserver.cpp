#include <QTcpServer>
#include <QTcpSocket>

#include "qjsonrpcsocket.h"
#include "qjsonrpcabstractserver_p.h"
#include "qjsonrpctcpserver.h"

class QJsonRpcTcpServerPrivate : public QJsonRpcAbstractServerPrivate
{
public:
    QHash<QTcpSocket*, QJsonRpcSocket*> socketLookup;
};

QJsonRpcTcpServer::QJsonRpcTcpServer(QObject *parent)
#if defined(USE_QT_PRIVATE_HEADERS)
    : QTcpServer(*new QJsonRpcTcpServerPrivate, parent)
#else
    : QTcpServer(parent),
      d_ptr(new QJsonRpcTcpServerPrivate)
#endif
{
}

QJsonRpcTcpServer::~QJsonRpcTcpServer()
{
    Q_D(QJsonRpcTcpServer);
    for (auto *socket : d->socketLookup.keys()) {
        socket->flush();
        socket->deleteLater();
    }
    d->socketLookup.clear();

    for (auto *client : d->clients)
        client->deleteLater();
    d->clients.clear();
}

bool QJsonRpcTcpServer::addService(QJsonRpcService *service)
{
    if (!QJsonRpcServiceProvider::addService(service))
        return false;

    connect(service, qOverload<const QJsonRpcMessage&>(&QJsonRpcService::notifyConnectedClients),
            this, qOverload<const QJsonRpcMessage&>(&QJsonRpcTcpServer::notifyConnectedClients));
    connect(service, qOverload<const QString&, const QJsonArray&>(&QJsonRpcService::notifyConnectedClients),
            this, qOverload<const QString&, const QJsonArray&>(&QJsonRpcTcpServer::notifyConnectedClients));
    return true;
}

bool QJsonRpcTcpServer::removeService(QJsonRpcService *service)
{
    if (!QJsonRpcServiceProvider::removeService(service))
        return false;

    disconnect(service, qOverload<const QJsonRpcMessage&>(&QJsonRpcService::notifyConnectedClients),
               this, qOverload<const QJsonRpcMessage&>(&QJsonRpcTcpServer::notifyConnectedClients));
    disconnect(service, qOverload<const QString&, const QJsonArray&>(&QJsonRpcService::notifyConnectedClients),
               this, qOverload<const QString&, const QJsonArray&>(&QJsonRpcTcpServer::notifyConnectedClients));
    return true;
}

int QJsonRpcTcpServer::connectedClientCount() const
{
    Q_D(const QJsonRpcTcpServer);
    return d->clients.size();
}

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
void QJsonRpcTcpServer::incomingConnection(qintptr socketDescriptor)
#else
void QJsonRpcTcpServer::incomingConnection(int socketDescriptor)
#endif
{
    Q_D(QJsonRpcTcpServer);
    QTcpSocket *tcpSocket = new QTcpSocket(this);
    if (!tcpSocket->setSocketDescriptor(socketDescriptor)) {
        qJsonRpcDebug() << Q_FUNC_INFO << "can't set socket descriptor";
        tcpSocket->deleteLater();
        return;
    }

    QIODevice *device = qobject_cast<QIODevice*>(tcpSocket);
    QJsonRpcSocket *socket = new QJsonRpcSocket(device, this);
    connect(socket, SIGNAL(messageReceived(QJsonRpcMessage)),
              this, SLOT(_q_processMessage(QJsonRpcMessage)));
    d->clients.append(socket);
    connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(_q_clientDisconnected()));
    d->socketLookup.insert(tcpSocket, socket);
    Q_EMIT clientConnected();
}

void QJsonRpcTcpServer::_q_clientDisconnected()
{
    Q_D(QJsonRpcTcpServer);
    QTcpSocket *tcpSocket = static_cast<QTcpSocket*>(sender());
    if (!tcpSocket) {
        qJsonRpcDebug() << Q_FUNC_INFO << "called with invalid socket";
        return;
    }

    if (d->socketLookup.contains(tcpSocket)) {
        QJsonRpcSocket *socket = d->socketLookup.take(tcpSocket);
        d->clients.removeAll(socket);
        socket->deleteLater();
    }

    tcpSocket->deleteLater();
    Q_EMIT clientDisconnected();
}

void QJsonRpcTcpServer::_q_processMessage(const QJsonRpcMessage &message)
{
    QJsonRpcSocket *socket = static_cast<QJsonRpcSocket*>(sender());
    if (!socket) {
        qJsonRpcDebug() << Q_FUNC_INFO << "called without service socket";
        return;
    }

    processMessage(socket, message);
}

void QJsonRpcTcpServer::notifyConnectedClients(const QJsonRpcMessage &message)
{
    Q_D(QJsonRpcTcpServer);
    d->_q_notifyConnectedClients(message);
}

void QJsonRpcTcpServer::notifyConnectedClients(const QString &method, const QJsonArray &params)
{
    Q_D(QJsonRpcTcpServer);
    d->_q_notifyConnectedClients(method, params);
}

#include "moc_qjsonrpctcpserver.cpp"
