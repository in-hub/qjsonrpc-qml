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
    connect(m_peer, SIGNAL(responseReceived(QJsonRpcResponse)),
              this, SLOT(responseReceived(QJsonRpcResponse)));
    connect(m_peer, SIGNAL(notificationReceived(QJsonRpcNotification)),
              this, SLOT(notificationReceived(QJsonRpcNotification)));

    m_peer->connectToPeer("testservice");
    m_peer->callRemoteMethod("agent.testMethod");
    m_peer->callRemoteMethod("agent.testMethodWithParams", "one", false, 10);
    m_peer->callRemoteMethod("agent.testMethodWithVariantParams", "one", false, 10, QVariant(2.5));
    m_peer->callRemoteMethod("agent.testMethodWithParamsAndReturnValue", "DogFace");

    m_peer->callRemoteMethod("agent.testMethodWithDefaultParameter", "blah");
    m_peer->callRemoteMethod("agent.testMethodWithDefaultParameter", "blah", "halb");

}

void Client::responseReceived(const QJsonRpcResponse &response)
{
    qDebug() << "response received: ";
    if (response.isError()) {
        qDebug() << "\tcode: " << response.error().code() << endl
                 << "\tmessage: " << response.error().message();
    } else {
        qDebug() << "\tresult: " << response.result();
    }
}

void Client::notificationReceived(const QJsonRpcNotification &notification)
{
}

