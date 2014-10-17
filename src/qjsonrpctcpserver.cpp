#include <QTcpServer>
#include <QTcpSocket>

#include "qjsonrpcsocket.h"
#include "qjsonrpcabstractserver_p.h"
#include "qjsonrpctcpserver.h"

class QJsonRpcTcpServerPrivate : public QJsonRpcAbstractServerPrivate
{
    Q_DECLARE_PUBLIC(QJsonRpcTcpServer)
public:
    QJsonRpcTcpServerPrivate(QJsonRpcTcpServer *q)
        : QJsonRpcAbstractServerPrivate(q),
          server(0)
    {
    }

    virtual void _q_processIncomingConnection();
    virtual void _q_clientDisconnected();

    QTcpServer *server;
    QHash<QTcpSocket*, QJsonRpcSocket*> socketLookup;
};

QJsonRpcTcpServer::QJsonRpcTcpServer(QObject *parent)
    : QJsonRpcAbstractServer(*new QJsonRpcTcpServerPrivate(this), parent)
{
}

QJsonRpcTcpServer::~QJsonRpcTcpServer()
{
    Q_D(QJsonRpcTcpServer);
    foreach (QTcpSocket *socket, d->socketLookup.keys())
        socket->deleteLater();
    d->socketLookup.clear();
}

bool QJsonRpcTcpServer::listen(const QHostAddress &address, quint16 port)
{
    Q_D(QJsonRpcTcpServer);
    if (!d->server) {
        d->server = new QTcpServer(this);
        connect(d->server, SIGNAL(newConnection()), this, SLOT(_q_processIncomingConnection()));
    }

    return d->server->listen(address, port);
}

void QJsonRpcTcpServer::close()
{
    Q_D(QJsonRpcTcpServer);
    d->server->close();
}

void QJsonRpcTcpServerPrivate::_q_processIncomingConnection()
{
    Q_Q(QJsonRpcTcpServer);
    QTcpSocket *tcpSocket = server->nextPendingConnection();
    if (!tcpSocket) {
        qJsonRpcDebug() << Q_FUNC_INFO << "nextPendingConnection is null";
        return;
    }

    QIODevice *device = qobject_cast<QIODevice*>(tcpSocket);
    QJsonRpcSocket *socket = new QJsonRpcSocket(device, q);
    QObject::connect(socket, SIGNAL(messageReceived(QJsonRpcMessage)),
                          q, SLOT(_q_processMessage(QJsonRpcMessage)));
    clients.append(socket);
    QObject::connect(tcpSocket, SIGNAL(disconnected()), q, SLOT(_q_clientDisconnected()));
    socketLookup.insert(tcpSocket, socket);
    Q_EMIT q->clientConnected();
}

void QJsonRpcTcpServerPrivate::_q_clientDisconnected()
{
    Q_Q(QJsonRpcTcpServer);
    QTcpSocket *tcpSocket = static_cast<QTcpSocket*>(q->sender());
    if (!tcpSocket) {
        qJsonRpcDebug() << Q_FUNC_INFO << "called with invalid socket";
        return;
    }

    if (socketLookup.contains(tcpSocket)) {
        QJsonRpcSocket *socket = socketLookup.take(tcpSocket);
        clients.removeAll(socket);
        socket->deleteLater();
    }

    tcpSocket->deleteLater();
    Q_EMIT q->clientDisconnected();
}

QString QJsonRpcTcpServer::errorString() const
{
    Q_D(const QJsonRpcTcpServer);
    return d->server->errorString();
}

#include "moc_qjsonrpctcpserver.cpp"
