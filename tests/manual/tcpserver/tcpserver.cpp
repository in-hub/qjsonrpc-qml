#include <QCoreApplication>
#include <QDesktopServices>
#include <QTcpServer>
#include <QFile>

#include "testservice.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QTcpServer tcpServer;
    if (!tcpServer.listen(QHostAddress::LocalHost, 5555)) {
        qDebug() << "can't start tcp server: " << tcpServer.errorString();
        return -1;
    }

    TestService service;
    QJsonRpcServiceProvider rpcServer(&tcpServer);
    rpcServer.addService(&service);
    return app.exec();
}
