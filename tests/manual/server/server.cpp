#include <QCoreApplication>

#include "testservice.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    TestService service;
    QJsonRpcServiceProvider server;
    server.addService(&service);
    if (!server.listen("testservice")) {
        qDebug() << "could not start server, aborting";
        return -1;
    }

    return app.exec();
}
