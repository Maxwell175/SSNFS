#include "clientmanager.h"

#include <QEventLoop>

ClientManager::ClientManager(QObject *parent) : QObject(parent)
{
    moveToThread(workThread);
    connect(workThread, SIGNAL(started()), this, SLOT(started()), Qt::QueuedConnection);
    workThread->start();
}

void ClientManager::started()
{

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        clients[i] = new ThreadedClient(i);
        clientStatuses[i] = ConnectionStatus::NotConnected;

        connect(clients[i], SIGNAL(statusChanged(ThreadedClient*,int,ConnectionStatus::ConnectionStatuses)), this, SLOT(clientStatusChanged(ThreadedClient*,int,ConnectionStatus::ConnectionStatuses)), Qt::QueuedConnection);
    }
}

void ClientManager::clientStatusChanged(ThreadedClient *client, int clientID, ConnectionStatus::ConnectionStatuses newStatus)
{
    (void) client;

    clientStatuses[clientID] = newStatus;

    if (newStatus == ConnectionStatus::Connected && theClient == NULL) {
        theClient = new FuseClient(this);
    }
}

int ClientManager::GetFreeClientId()
{
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (clientStatuses[i] == ConnectionStatus::Connected) {
            return i;
        }
    }
    return -1;
}

ThreadedClient* ClientManager::GetAClient()
{
    int freeClientId = -1;
    // Wait for a free client.
    while ((freeClientId = GetFreeClientId()) < 0) {
        QList<QMetaObject::Connection> slotConns;
        // Make an event loop that will wait for a statusChanged signal from any of the clients.
        QEventLoop loop;
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            slotConns.append(connect(clients[i], SIGNAL(statusChanged(ThreadedClient*,int,ConnectionStatus::ConnectionStatuses)), &loop, SLOT(quit())));
        }
        loop.exec();

        // Disconnect the connections.
        foreach (QMetaObject::Connection conn, slotConns) {
            disconnect(conn);
        }
    }

    QMetaObject::invokeMethod(clients[freeClientId], "LockClient", Qt::QueuedConnection);
    clientStatuses[freeClientId] = ConnectionStatus::InUse;
    return clients[freeClientId];
}
