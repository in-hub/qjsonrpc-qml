#include <QCoreApplication>

#include "qjsonrpcpeer.h"
#include "testservice.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    TestService service;
    QJsonRpcPeer peer;
    peer.addService(&service);
    if (!peer.listenForPeers("testservice")) {
        qDebug() << "could not start server, aborting";
        return -1;
    }

    return app.exec();
}
