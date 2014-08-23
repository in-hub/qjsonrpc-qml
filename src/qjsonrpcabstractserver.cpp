#include <QMetaObject>
#include <QMetaClassInfo>
#include <QDebug>

#include "qjsonrpcservice_p.h"
#include "qjsonrpcservice.h"
#include "qjsonrpcsocket.h"
#include "qjsonrpcabstractserver_p.h"
#include "qjsonrpcabstractserver.h"

QJsonRpcServiceProvider::QJsonRpcServiceProvider()
    : d(new QJsonRpcServiceProviderPrivate)
{
}

QJsonRpcServiceProvider::~QJsonRpcServiceProvider()
{
}

QByteArray QJsonRpcServiceProviderPrivate::serviceName(QJsonRpcService *service)
{
    const QMetaObject *mo = service->metaObject();
    for (int i = 0; i < mo->classInfoCount(); i++) {
        const QMetaClassInfo mci = mo->classInfo(i);
        if (mci.name() == QLatin1String("serviceName"))
            return mci.value();
    }

    return QByteArray(mo->className()).toLower();
}

bool QJsonRpcServiceProvider::addService(QJsonRpcService *service)
{
    QByteArray serviceName = d->serviceName(service);
    if (serviceName.isEmpty()) {
        qJsonRpcDebug() << Q_FUNC_INFO << "service added without serviceName classinfo, aborting";
        return false;
    }

    if (d->services.contains(serviceName)) {
        qJsonRpcDebug() << Q_FUNC_INFO << "service with name " << serviceName << " already exist";
        return false;
    }

    service->d_func()->cacheInvokableInfo();
    d->services.insert(serviceName, service);
    if (!service->parent())
        d->cleanupHandler.add(service);
    return true;
}

bool QJsonRpcServiceProvider::removeService(QJsonRpcService *service)
{
    QByteArray serviceName = d->serviceName(service);
    if (!d->services.contains(serviceName)) {
        qJsonRpcDebug() << Q_FUNC_INFO << "can not find service with name " << serviceName;
        return false;
    }

    d->cleanupHandler.remove(d->services.value(serviceName));
    d->services.remove(serviceName);
    return true;
}

void QJsonRpcServiceProvider::processMessage(QJsonRpcAbstractSocket *socket, const QJsonRpcMessage &message)
{
    switch (message.type()) {
        case QJsonRpcMessage::Request:
        case QJsonRpcMessage::Notification: {
            QByteArray serviceName = message.method().section(".", 0, -2).toLatin1();
            if (serviceName.isEmpty() || !d->services.contains(serviceName)) {
                if (message.type() == QJsonRpcMessage::Request) {
                    QJsonRpcMessage error =
                        message.createErrorResponse(QJsonRpc::MethodNotFound,
                            QString("service '%1' not found").arg(serviceName.constData()));
                    socket->notify(error);
                }
            } else {
                QJsonRpcService *service = d->services.value(serviceName);
                service->d_func()->socket = socket;
                if (message.type() == QJsonRpcMessage::Request)
                    QObject::connect(service, SIGNAL(result(QJsonRpcMessage)),
                                      socket, SLOT(notify(QJsonRpcMessage)), Qt::UniqueConnection);
                service->dispatch(message);
            }
        }
        break;

        case QJsonRpcMessage::Response:
            // we don't handle responses in the provider
            break;

        default: {
            QJsonRpcMessage error =
                message.createErrorResponse(QJsonRpc::InvalidRequest, QString("invalid request"));
            socket->notify(error);
            break;
        }
    };
}

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

#if QT_VERSION >= 0x050100 || QT_VERSION <= 0x050000
QJsonDocument::JsonFormat QJsonRpcAbstractServer::wireFormat() const
{
    Q_D(const QJsonRpcAbstractServer);
    return d->format;
}

void QJsonRpcAbstractServer::setWireFormat(QJsonDocument::JsonFormat format)
{
    Q_D(QJsonRpcAbstractServer);
    d->format = format;
}
#endif

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
