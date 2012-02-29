#include <QCoreApplication>

#include "client.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Client client;
    client.run();
    return app.exec();
}
