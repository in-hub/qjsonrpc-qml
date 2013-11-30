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
#include <QCoreApplication>
#include <QAuthenticator>
#include <QStringList>
#include <QDebug>

#include "qjsonrpchttpclient.h"

class HttpClient : public QJsonRpcHttpClient
{
    Q_OBJECT
public:
    HttpClient(const QString &endpoint, QObject *parent = 0)
        : QJsonRpcHttpClient(endpoint, parent)
    {
    }

private Q_SLOTS:
    virtual void handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator * authenticator)
    {
        Q_UNUSED(reply)
        authenticator->setUser("bitcoinrpc");
        authenticator->setPassword("232fb3276bbb7437d265298ea48bdc46");
    }
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (app.arguments().size() < 2) {
        qDebug() << "usage: " << argv[0] << " <command> <arguments>";
        return -1;
    }

    HttpClient client("http://127.0.0.1:8332");
    QJsonRpcMessage message = QJsonRpcMessage::createRequest(app.arguments().at(1));
    QJsonRpcMessage response = client.sendMessageBlocking(message);
    qDebug() << response;
    if (response.type() == QJsonRpcMessage::Error)
        qDebug() << response.errorData();
}

#include "main.moc"
