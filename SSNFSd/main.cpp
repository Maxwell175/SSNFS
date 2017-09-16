#include <QCoreApplication>

#include "ssnfsserver.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    SSNFSServer server;

    server.listen(QHostAddress::Any, 2050);

    app.exec();
}
