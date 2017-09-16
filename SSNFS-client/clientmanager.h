#ifndef ClientManager_H
#define ClientManager_H

#include <QObject>
#include <QThread>
#include "fuseclient.h"

#include "threadedclient.h"

class FuseClient;

#define MAX_CONNECTIONS 1

class ClientManager : public QObject
{
    Q_OBJECT
public:
    explicit ClientManager(QObject *parent = 0);

private:
    int GetFreeClientId();

    QThread *workThread = new QThread();

    FuseClient *theClient = NULL;

    ThreadedClient* clients[MAX_CONNECTIONS];

    ConnectionStatus::ConnectionStatuses clientStatuses[MAX_CONNECTIONS];

signals:

private slots:
    void started();
    void clientStatusChanged(ThreadedClient *client, int clientID, ConnectionStatus::ConnectionStatuses newStatus);

public slots:
    ThreadedClient* GetAClient();

};

#endif // ClientManager_H
