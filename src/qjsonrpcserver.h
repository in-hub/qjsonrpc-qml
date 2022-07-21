#ifndef QJSONRPCSERVER_H
#define QJSONRPCSERVER_H

#include <QLocalServer>
#include <QTcpServer>

class QJsonRpcTcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit QJsonRpcTcpServer(QObject *parent = nullptr) : QTcpServer(parent) { }

Q_SIGNALS:
    void newIncomingConnection(qintptr socketDescriptor);

protected:
    void incomingConnection(qintptr socketDescriptor) override
    {
        Q_EMIT newIncomingConnection(socketDescriptor);
    }
};

class QJsonRpcLocalServer : public QLocalServer
{
    Q_OBJECT
public:
    explicit QJsonRpcLocalServer(QObject *parent = nullptr) : QLocalServer(parent) { }

Q_SIGNALS:
    void newIncomingConnection(qintptr socketDescriptor);

protected:
    void incomingConnection(quintptr socketDescriptor) override
    {
        Q_EMIT newIncomingConnection(qintptr(socketDescriptor));
    }
};

#endif
