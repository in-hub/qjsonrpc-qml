#include <QDebug>

#include "qjsonrpcmessage_p.h"

int QJsonRpcMessagePrivate::uniqueRequestCounter = 0;
QJsonRpcMessagePrivate::QJsonRpcMessagePrivate()
    : type(QJsonRpcMessage::Invalid),
      object(0)
{
}

QJsonRpcMessagePrivate::~QJsonRpcMessagePrivate()
{
}

QJsonRpcMessage::QJsonRpcMessage()
    : d(new QJsonRpcMessagePrivate)
{
}

QJsonRpcMessage::QJsonRpcMessage(const QJsonRpcMessage &other)
    : d(other.d)
{
}

QJsonRpcMessage::~QJsonRpcMessage()
{
}

QJsonRpcMessage &QJsonRpcMessage::operator=(const QJsonRpcMessage &other)
{
    d = other.d;
    return *this;
}

QJsonRpcMessage::QJsonRpcMessage(const QJsonObject &message)
    : d(new QJsonRpcMessagePrivate)
{
    if (message.contains("id")) {
        if (message.contains("result") || message.contains("error")) {
            d->object = new QJsonObject(message);
            if (message.contains("error"))
                d->type = QJsonRpcMessage::Error;
            else
                d->type = QJsonRpcMessage::Response;
        } else if (message.contains("method")) {
            d->object = new QJsonObject(message);
            d->type = QJsonRpcMessage::Request;
        }
    } else {
        if (message.contains("method")) {
            d->object = new QJsonObject(message);
            d->type = QJsonRpcMessage::Notification;
        }
    }
}

QJsonObject QJsonRpcMessage::toObject() const
{
    if (d->object)
        return QJsonObject(*d->object);
    return QJsonObject();
}

QJsonRpcMessage::Type QJsonRpcMessage::type() const
{
    return d->type;
}

QJsonRpcMessage QJsonRpcMessagePrivate::createBasicRequest(const QString &method, const QVariantList &params)
{
    QJsonRpcMessage request;
    QJsonObject *object = new QJsonObject;
    object->insert("jsonrpc", QLatin1String("2.0"));
    object->insert("method", method);
    if (!params.isEmpty())
        object->insert("params", QJsonArray::fromVariantList(params));
    request.d->object = object;
    return request;
}

QJsonRpcMessage QJsonRpcMessage::createRequest(const QString &method, const QVariantList &params)
{
    QJsonRpcMessage request = QJsonRpcMessagePrivate::createBasicRequest(method, params);
    request.d->type = QJsonRpcMessage::Request;
    QJsonRpcMessagePrivate::uniqueRequestCounter++;
    request.d->object->insert("id", QJsonRpcMessagePrivate::uniqueRequestCounter);
    return request;
}

QJsonRpcMessage QJsonRpcMessage::createRequest(const QString &method, const QVariant &param)
{
    return createRequest(method, QVariantList() << param);
}

QJsonRpcMessage QJsonRpcMessage::createNotification(const QString &method, const QVariantList &params)
{
    QJsonRpcMessage notification = QJsonRpcMessagePrivate::createBasicRequest(method, params);
    notification.d->type = QJsonRpcMessage::Notification;
    return notification;
}

QJsonRpcMessage QJsonRpcMessage::createResponse(const QVariant &result) const
{
    QJsonRpcMessage response;
    response.d->type = QJsonRpcMessage::Response;
    QJsonObject *object = new QJsonObject;
    object->insert("jsonrpc", QLatin1String("2.0"));
    object->insert("id", d->object->value("id"));
    object->insert("result", QJsonValue::fromVariant(result));
    response.d->object = object;
    return response;
}

QJsonRpcMessage QJsonRpcMessage::createErrorResponse(QJsonRpc::ErrorCode code, const QString &message, const QVariant &data) const
{
    QJsonRpcMessage response;

    QJsonObject error;
    error.insert("code", code);
    if (!message.isEmpty())
        error.insert("message", message);
    if (data.isValid())
        error.insert("data", QJsonValue::fromVariant(data));

    response.d->type = QJsonRpcMessage::Error;
    QJsonObject *object = new QJsonObject;
    object->insert("jsonrpc", QLatin1String("2.0"));
    object->insert("id", d->object->value("id"));
    object->insert("error", error);
    response.d->object = object;
    return response;
}

int QJsonRpcMessage::id() const
{
    if (d->type == QJsonRpcMessage::Notification || !d->object)
        return -1;
    return d->object->value("id").toVariant().toInt();
}

QString QJsonRpcMessage::method() const
{
    if (d->type != QJsonRpcMessage::Request || !d->object)
        return QString();

    return d->object->value("method").toString();
}

QVariantList QJsonRpcMessage::params() const
{
    if (d->type != QJsonRpcMessage::Request || !d->object)
        return QVariantList();

    return d->object->value("params").toVariant().toList();
}

QVariant QJsonRpcMessage::result() const
{
    if (d->type != QJsonRpcMessage::Response || !d->object)
        return QVariant();

    return d->object->value("result").toVariant();
}

int QJsonRpcMessage::errorCode() const
{
    if (d->type != QJsonRpcMessage::Error || !d->object)
        return 0;

    QJsonObject error = d->object->value("error").toObject();
    return error.value("code").toVariant().toInt();
}

QString QJsonRpcMessage::errorMessage() const
{
    if (d->type != QJsonRpcMessage::Error || !d->object)
        return 0;

    QJsonObject error = d->object->value("error").toObject();
    return error.value("message").toString();
}

QVariant QJsonRpcMessage::errorData() const
{
    if (d->type != QJsonRpcMessage::Error || !d->object)
        return 0;

    QJsonObject error = d->object->value("error").toObject();
    return error.value("data").toVariant();
}

static QDebug operator<<(QDebug dbg, QJsonRpcMessage::Type type)
{
    switch (type) {
    case QJsonRpcMessage::Request:
        return dbg << "QJsonRpcMessage::Request";
    case QJsonRpcMessage::Response:
        return dbg << "QJsonRpcMessage::Response";
    case QJsonRpcMessage::Notification:
        return dbg << "QJsonRpcMessage::Notification";
    case QJsonRpcMessage::Error:
        return dbg << "QJsonRpcMessage::Error";
    default:
        return dbg << "QJsonRpcMessage::Invalid";
    }
}

QDebug operator<<(QDebug dbg, const QJsonRpcMessage &msg)
{
    dbg.nospace() << "QJsonRpcMessage(type=" << msg.type();
    if (msg.type() != QJsonRpcMessage::Notification) {
        dbg.nospace() << ", id=" << msg.id();
    }

    if (msg.type() == QJsonRpcMessage::Request) {
        dbg.nospace() << ", method=" << msg.method()
                      << ", params=" << msg.params();
    } else if (msg.type() == QJsonRpcMessage::Response) {
        dbg.nospace() << ", result=" << msg.result();
    } else if (msg.type() == QJsonRpcMessage::Error) {
        dbg.nospace() << ", code=" << msg.errorCode()
                      << ", message=" << msg.errorMessage()
                      << ", data=" << msg.errorData();
    }
    dbg.nospace() << ")";
    return dbg.space();
}
