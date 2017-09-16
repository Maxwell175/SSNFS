#include <QCoreApplication>

#include "fuseclient.h"

int main(int argc, char *argv[])
{
    qputenv("QT_LOGGING_RULES", "qt.network.ssl.warning=false");

    QCoreApplication app(argc, argv);

    qRegisterMetaType<fuse_conn_info*>("fuse_conn_info*");
    qRegisterMetaType<fs_stat*>("fs_stat*");

    FuseClient client;

    app.exec();
}
