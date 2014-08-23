/*
 * Copyright (C) 2012-2013 Matt Broadstone
 * Contact: http://bitbucket.org/devonit/qjsonrpc
 *
 * This file is part of the QJsonRpc Library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#else
#include "json/qjsondocument.h"
#endif

#include "qjsonrpcservicereply_p.h"
#include "qjsonrpchttpclient.h"

class QJsonRpcHttpReplyPrivate : public QJsonRpcServiceReplyPrivate
{
public:
    QJsonRpcMessage request;
    QNetworkReply *reply;
};

class QJsonRpcHttpReply : public QJsonRpcServiceReply
{
    Q_OBJECT
public:
    QJsonRpcHttpReply(const QJsonRpcMessage &request,
                      QNetworkReply *reply, QObject *parent = 0)
        : QJsonRpcServiceReply(*new QJsonRpcHttpReplyPrivate, parent)
    {
        Q_D(QJsonRpcHttpReply);
        d->request = request;
        d->reply = reply;
        connect(d->reply, SIGNAL(finished()), this, SLOT(networkReplyFinished()));
        connect(d->reply, SIGNAL(error(QNetworkReply::NetworkError)),
                    this, SLOT(networkReplyError(QNetworkReply::NetworkError)));
    }

    virtual ~QJsonRpcHttpReply() {}

private Q_SLOTS:
    void networkReplyFinished()
    {
        Q_D(QJsonRpcHttpReply);
        QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
        if (!reply) {
            qJsonRpcDebug() << Q_FUNC_INFO << "invalid reply";
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            // this should be handled by the networkReplyError slot
        } else {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isEmpty() || doc.isNull() || !doc.isObject()) {
                d->response =
                    d->request.createErrorResponse(QJsonRpc::ParseError,
                                                   "unable to process incoming JSON data",
                                                   QString::fromUtf8(data));
            } else {
                qJsonRpcDebug() << "received: " << doc.toJson();
                QJsonRpcMessage response = QJsonRpcMessage(doc.object());
                if (d->request.type() == QJsonRpcMessage::Request &&
                    d->request.id() != response.id()) {
                    d->response =
                        d->request.createErrorResponse(QJsonRpc::InternalError,
                                                       "invalid response id",
                                                       QString::fromUtf8(data));
                } else {
                    d->response = response;
                }
            }
        }

        Q_EMIT finished();
    }

    void networkReplyError(QNetworkReply::NetworkError code)
    {
        Q_D(QJsonRpcHttpReply);
        QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
        if (!reply) {
            qJsonRpcDebug() << Q_FUNC_INFO << "invalid reply";
            return;
        }

        if (code == QNetworkReply::NoError)
            return;

        d->response = d->request.createErrorResponse(QJsonRpc::InternalError,
                                       "error with http request",
                                       reply->errorString());
        Q_EMIT finished();
    }

private:
    Q_DISABLE_COPY(QJsonRpcHttpReply)
    Q_DECLARE_PRIVATE(QJsonRpcHttpReply)

};

#if defined(USE_QT_PRIVATE_HEADERS)
#include <private/qobject_p.h>

class QJsonRpcHttpClientPrivate : public QObjectPrivate
#else
class QJsonRpcHttpClientPrivate
#endif
{
public:
    void initializeNetworkAccessManager(QJsonRpcHttpClient *client) {
        QObject::connect(networkAccessManager, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
                client, SLOT(handleAuthenticationRequired(QNetworkReply*,QAuthenticator*)));
        QObject::connect(networkAccessManager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),
                client, SLOT(handleSslErrors(QNetworkReply*,QList<QSslError>)));
    }

    QNetworkReply *writeMessage(const QJsonRpcMessage &message) {
        QNetworkRequest request(endPoint);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QByteArray data = QJsonDocument(message.toObject()).toJson();
        qJsonRpcDebug() << "sending: " << data;
        return networkAccessManager->post(request, data);
    }

    QUrl endPoint;
    QNetworkAccessManager *networkAccessManager;
};

QJsonRpcHttpClient::QJsonRpcHttpClient(QObject *parent)
#if defined(USE_QT_PRIVATE_HEADERS)
    : QObject(*new QJsonRpcHttpClientPrivate, parent)
#else
    : QObject(parent),
      d_ptr(new QJsonRpcHttpClientPrivate)
#endif
{
    Q_D(QJsonRpcHttpClient);
    d->networkAccessManager = new QNetworkAccessManager(this);
    d->initializeNetworkAccessManager(this);
}

QJsonRpcHttpClient::QJsonRpcHttpClient(QNetworkAccessManager *manager, QObject *parent)
#if defined(USE_QT_PRIVATE_HEADERS)
    : QObject(*new QJsonRpcHttpClientPrivate, parent)
#else
    : QObject(parent),
      d_ptr(new QJsonRpcHttpClientPrivate)
#endif
{
    Q_D(QJsonRpcHttpClient);
    d->networkAccessManager = manager;
    d->initializeNetworkAccessManager(this);
}

QJsonRpcHttpClient::QJsonRpcHttpClient(const QString &endPoint, QObject *parent)
#if defined(USE_QT_PRIVATE_HEADERS)
    : QObject(*new QJsonRpcHttpClientPrivate, parent)
#else
    : QObject(parent),
      d_ptr(new QJsonRpcHttpClientPrivate)
#endif
{
    Q_D(QJsonRpcHttpClient);
    d->endPoint = QUrl::fromUserInput(endPoint);
    d->networkAccessManager = new QNetworkAccessManager(this);
    d->initializeNetworkAccessManager(this);
}

QJsonRpcHttpClient::~QJsonRpcHttpClient()
{
}

QUrl QJsonRpcHttpClient::endPoint() const
{
    Q_D(const QJsonRpcHttpClient);
    return d->endPoint;
}

void QJsonRpcHttpClient::setEndPoint(const QUrl &endPoint)
{
    Q_D(QJsonRpcHttpClient);
    d->endPoint = endPoint;
}

void QJsonRpcHttpClient::setEndPoint(const QString &endPoint)
{
    Q_D(QJsonRpcHttpClient);
    d->endPoint = QUrl::fromUserInput(endPoint);
}

QNetworkAccessManager *QJsonRpcHttpClient::networkAccessManager()
{
    Q_D(QJsonRpcHttpClient);
    return d->networkAccessManager;
}

void QJsonRpcHttpClient::notify(const QJsonRpcMessage &message)
{
    Q_D(QJsonRpcHttpClient);
    if (d->endPoint.isEmpty()) {
        qJsonRpcDebug() << Q_FUNC_INFO << "invalid endpoint specified";
        return;
    }

    QNetworkReply *reply = d->writeMessage(message);
    connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));

    // NOTE: we might want to connect this to a local slot to track errors
    //       for debugging later?
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), reply, SLOT(deleteLater()));
}

QJsonRpcServiceReply *QJsonRpcHttpClient::sendMessage(const QJsonRpcMessage &message)
{
    Q_D(QJsonRpcHttpClient);
    if (d->endPoint.isEmpty()) {
        qJsonRpcDebug() << Q_FUNC_INFO << "invalid endpoint specified";
        return 0;
    }

    QNetworkReply *reply = d->writeMessage(message);
    return new QJsonRpcHttpReply(message, reply);
}

QJsonRpcMessage QJsonRpcHttpClient::sendMessageBlocking(const QJsonRpcMessage &message, int msecs)
{
    QJsonRpcServiceReply *reply = sendMessage(message);
    QScopedPointer<QJsonRpcServiceReply> replyPtr(reply);

    QEventLoop responseLoop;
    connect(reply, SIGNAL(finished()), &responseLoop, SLOT(quit()));
    QTimer::singleShot(msecs, &responseLoop, SLOT(quit()));
    responseLoop.exec();

    if (!reply->response().isValid())
        return message.createErrorResponse(QJsonRpc::TimeoutError, "request timed out");
    return reply->response();
}

void QJsonRpcHttpClient::handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_UNUSED(reply)
    Q_UNUSED(authenticator)
}

void QJsonRpcHttpClient::handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    Q_UNUSED(errors)
    reply->ignoreSslErrors();
}

#include "qjsonrpchttpclient.moc"
