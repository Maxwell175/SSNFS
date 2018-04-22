#ifndef SSNFSWORKER_H
#define SSNFSWORKER_H

#include <QThread>
#include <QSqlDatabase>
#include <QSslSocket>

#include <common.h>

enum ClientStatus {
    WaitingForHello,
    WaitingForShare,
    WaitingForOperation,
    InOperation
};

class SSNFSWorker : public QThread
{
    Q_OBJECT
public:
    SSNFSWorker(int socketDescriptor, QObject *parent);
    ~SSNFSWorker() {
        socket->deleteLater();
    }

    void run() override;

    QString getPerms(QString path, qint32 uid);
    void ReadyToRead();
    void processHttpRequest();

    QSqlDatabase configDB;

    QSslSocket *socket;
    ClientStatus status = WaitingForHello;
    Common::Operation operation;
    bool working = false;
    QHash<int, int> fds;
    QTime timer;

    qint64 clientKey = -1;
    QString clientName;
    qint64 shareKey = -1;
    QString shareName;
    QString defaultPerms;
    QString localPath;

private:
    int socketDescriptor;
};

#endif // SSNFSWORKER_H
