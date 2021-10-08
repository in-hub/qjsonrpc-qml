#include <QObjectCleanupHandler>
#include <QMetaObject>
#include <QMetaClassInfo>
#include <QDebug>

#include "qjsonrpcservice.h"
#include "qjsonrpcservice_p.h"
#include "qjsonrpcsocket.h"
#include "qjsonrpcserviceprovider.h"

class QJsonRpcServiceProviderPrivate
{
public:
    QByteArray serviceName(QJsonRpcService *service);

    QHash<QByteArray, QJsonRpcService*> services;
    QObjectCleanupHandler cleanupHandler;

};

QJsonRpcServiceProvider::QJsonRpcServiceProvider()
    : d(new QJsonRpcServiceProviderPrivate)
{
}

QJsonRpcServiceProvider::~QJsonRpcServiceProvider()
{
}

QByteArray QJsonRpcServiceProviderPrivate::serviceName(QJsonRpcService *service)
{
    if (!service->property("serviceName").isNull()) {
        return service->property("serviceName").toByteArray();
    }
    const QMetaObject *mo = service->metaObject();
    for (int i = 0; i < mo->classInfoCount(); i++) {
        const QMetaClassInfo mci = mo->classInfo(i);
        if (QLatin1String(mci.name()) == QLatin1String("serviceName"))
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

void QJsonRpcServiceProvider::removeAllServices()
{
    for (auto service : d->services) {
        d->cleanupHandler.remove(service);
    }
    d->services.clear();
}

void QJsonRpcServiceProvider::processMessage(QJsonRpcAbstractSocket *socket, const QJsonRpcMessage &message)
{
    switch (message.type()) {
        case QJsonRpcMessage::Request:
        case QJsonRpcMessage::Notification: {
            QByteArray serviceName = message.method().section(QLatin1Char('.'), 0, -2).toLatin1();
            if (serviceName.isEmpty() || !d->services.contains(serviceName)) {
                if (message.type() == QJsonRpcMessage::Request) {
                    QJsonRpcMessage error =
                        message.createErrorResponse(QJsonRpc::MethodNotFound,
                            QStringLiteral("service '%1' not found").arg(QString::fromUtf8(serviceName)));
                    socket->notify(error);
                }
            } else {
                QJsonRpcService *service = d->services.value(serviceName);
                service->d_func()->currentRequest = QJsonRpcServiceRequest(message, socket);
                if (message.type() == QJsonRpcMessage::Request)
                    QObject::connect(service, &QJsonRpcService::result,
                                      socket, &QJsonRpcAbstractSocket::notify, Qt::UniqueConnection);
                QJsonRpcMessage response = service->dispatch(message);
                if (response.isValid())
                    socket->notify(response);
            }
        }
        break;

        case QJsonRpcMessage::Response:
            // we don't handle responses in the provider
            break;

        default: {
            QJsonRpcMessage error =
                message.createErrorResponse(QJsonRpc::InvalidRequest, QStringLiteral("invalid request"));
            socket->notify(error);
            break;
        }
    };
}
