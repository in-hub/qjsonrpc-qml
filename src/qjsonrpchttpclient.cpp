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
#include "qjsonrpchttpclient.h"

class QJsonRpcHttpReplyPrivate
{
public:
    QNetworkReply *reply;

};

QJsonRpcHttpReply::QJsonRpcHttpReply(QNetworkReply *reply, QObject *parent)
    : QJsonRpcServiceReply(parent),
      d_ptr(new QJsonRpcHttpReplyPrivate)
{
    Q_D(QJsonRpcHttpReply);
    d->reply = reply;
}

QJsonRpcHttpReply::~QJsonRpcHttpReply()
{
}

void QJsonRpcHttpReply::networkReplyFinished()
{
    // parse result of QNetworkReply, then
    Q_EMIT finished();
}

void QJsonRpcHttpReply::networkReplyerror(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code)
    // set result to some error, then
    Q_EMIT finished();
}

class QJsonRpcHttpClientPrivate
{
public:
    QNetworkAccessManager *networkAccessManager;
};

QJsonRpcHttpClient::QJsonRpcHttpClient(QObject *parent)
    : QObject(parent),
      d_ptr(new QJsonRpcHttpClientPrivate)
{
    Q_D(QJsonRpcHttpClient);
    d->networkAccessManager = new QNetworkAccessManager(this);
    connect(d->networkAccessManager, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
            this, SLOT(handleAuthenticationRequired(QNetworkReply*,QAuthenticator*)));
    connect(d->networkAccessManager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),
            this, SLOT(handleSslErrors(QNetworkReply*,QList<QSslError>)));
}

QJsonRpcHttpClient::QJsonRpcHttpClient(QNetworkAccessManager *manager, QObject *parent)
    : QObject(parent),
      d_ptr(new QJsonRpcHttpClientPrivate)
{
    Q_D(QJsonRpcHttpClient);
    d->networkAccessManager = manager;
    connect(d->networkAccessManager, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
            this, SLOT(handleAuthenticationRequired(QNetworkReply*,QAuthenticator*)));
    connect(d->networkAccessManager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),
            this, SLOT(handleSslErrors(QNetworkReply*,QList<QSslError>)));

}

QJsonRpcHttpClient::~QJsonRpcHttpClient()
{
}

QNetworkAccessManager *QJsonRpcHttpClient::networkAccessManager()
{
    Q_D(QJsonRpcHttpClient);
    return d->networkAccessManager;
}

void QJsonRpcHttpClient::notify(const QJsonRpcMessage &message)
{
    Q_UNUSED(message)
    // TODO
}

QJsonRpcMessage QJsonRpcHttpClient::sendMessageBlocking(const QJsonRpcMessage &message, int msecs)
{
    Q_UNUSED(message)
    Q_UNUSED(msecs)
    // TODO

    return QJsonRpcMessage();
}

QJsonRpcServiceReply *QJsonRpcHttpClient::sendMessage(const QJsonRpcMessage &message)
{
    Q_UNUSED(message)
    // TODO

    return 0;
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

