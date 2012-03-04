#include "qjsonrpcservice.h"
#include "json/qjsonvalue.h"
#include "client.h"

Client::Client(QObject *parent)
    : QObject(parent),
      m_client(0)
{
}

void Client::run()
{
    QLocalSocket *socket = new QLocalSocket(this);
    connect(socket, SIGNAL(connected()), this, SLOT(clientConnected()));
    socket->connectToServer("testService");
}

void Client::clientConnected()
{
    QLocalSocket *socket = static_cast<QLocalSocket *>(sender());
    if (!socket) {
        qDebug() << "unexpected connection message";
        return;
    }

    m_client = new QJsonRpcServiceSocket(socket, this);
    connect(m_client, SIGNAL(messageReceived(QJsonRpcMessage)), this, SLOT(processMessage(QJsonRpcMessage)));

    m_client->invokeRemoteMethod("agent.testMethod");
    m_client->invokeRemoteMethod("agent.testMethodWithParams", "one", false, 10);
    m_client->invokeRemoteMethod("agent.testMethodWithVariantParams", "one", false, 10, QVariant(2.5));
    m_client->invokeRemoteMethod("agent.testMethodWithParamsAndReturnValue", "matt");

//    m_client->callRemoteMethod("agent.testMethodWithDefaultParameter", "blah");
//    m_client->callRemoteMethod("agent.testMethodWithDefaultParameter", "blah", "halb");

    // test bulk messages
    QJsonRpcMessage first = QJsonRpcMessage::createRequest("agent.testMethodWithParamsAndReturnValue", "testSendMessage");
    m_client->sendMessage(first);

    QJsonRpcMessage second = QJsonRpcMessage::createRequest("agent.testMethodWithParamsAndReturnValue", "testSendMessages1");
    QJsonRpcMessage third = QJsonRpcMessage::createRequest("agent.testMethodWithParamsAndReturnValue", "testSendMessages2");
    m_client->sendMessage(QList<QJsonRpcMessage>() << second << third);
}

void Client::processMessage(const QJsonRpcMessage &message)
{
    qDebug() << "message received: " << message;
}
