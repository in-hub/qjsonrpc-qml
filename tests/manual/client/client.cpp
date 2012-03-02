#include "qjsonvalue.h"
#include "client.h"

Client::Client(QObject *parent)
    : QObject(parent),
      m_peer(0)
{
}

void Client::run()
{
    m_peer = new QJsonRpcPeer(this);
    connect(m_peer, SIGNAL(messageReceived(QJsonRpcMessage)), this, SLOT(processMessage(QJsonRpcMessage)));

    m_peer->connectToPeer("testservice");
    m_peer->callRemoteMethod("agent.testMethod");
    m_peer->callRemoteMethod("agent.testMethodWithParams", "one", false, 10);
    m_peer->callRemoteMethod("agent.testMethodWithVariantParams", "one", false, 10, QVariant(2.5));
    m_peer->callRemoteMethod("agent.testMethodWithParamsAndReturnValue", "DogFace");

//    m_peer->callRemoteMethod("agent.testMethodWithDefaultParameter", "blah");
//    m_peer->callRemoteMethod("agent.testMethodWithDefaultParameter", "blah", "halb");

    // test bulk messages
    QJsonRpcMessage first = QJsonRpcMessage::createRequest("agent.testMethod");
    QJsonRpcMessage second = QJsonRpcMessage::createRequest("agent.testMethodWithParams", QVariantList() << "one" << false << 10);
    m_peer->sendMessages(QList<QJsonRpcMessage>() << first << second);

}

void Client::processMessage(const QJsonRpcMessage &message)
{
    qDebug() << "message received: " << message;
}
