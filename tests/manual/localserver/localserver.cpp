#include <QCoreApplication>
#include <QDesktopServices>
#include <QLocalServer>
#include <QFile>

#include "testservice.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    // setup local server
    QString serviceName;
#if defined(Q_OS_WIN)
    QDir tempDirectory(QDesktopServices::storageLocation(QDesktopServices::TempLocation));
    serviceName = tempDirectory.absoluteFilePath("testservice");
#else
    serviceName = "/tmp/testservice";
#endif

    if (QFile::exists(serviceName)) {
        if (!QFile::remove(serviceName)) {
            qDebug() << "couldn't delete temporary service";
            return -1;
        }
    }

    QLocalServer localServer;
    if (!localServer.listen(serviceName)) {
        qDebug() << "could not start server, aborting";
        return -1;
    }

    TestService service;
    QJsonRpcServiceProvider rpcServer(&localServer);
    rpcServer.addService(&service);
    return app.exec();
}
