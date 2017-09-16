#ifndef THREADEDCLIENT_H
#define THREADEDCLIENT_H

#include <QObject>
#include <QThread>
#include <QSslSocket>
#include <QTimer>

namespace ConnectionStatus {
enum ConnectionStatuses {
    NotConnected,
    Connected,
    InUse
};
}

class ThreadedClient : public QObject
{
    Q_OBJECT
public:
    explicit ThreadedClient(int ClientId, QObject *parent = 0);

    ConnectionStatus::ConnectionStatuses ClientStatus = ConnectionStatus::NotConnected;
    int ClientId;

private:
    QThread *workThread = new QThread();
    QSslSocket *socket;

    QTimer socketResetTimer;

signals:
    void statusChanged(ThreadedClient *sender, int ClientId, ConnectionStatus::ConnectionStatuses newStatus);

private slots:
    void startSocket();

    void resetSocket();

    void socketDisconnected();
    void socketError(QAbstractSocket::SocketError error);
    void sslErrorOccurred(QList<QSslError> errors);

public slots:
    void LockClient();
    void UnlockClient();
    /**
     * @brief SendData
     * Properly sends data using the protocol.
     * @param socket The socket to send from.
     * @param data The data to transmit.
     * @return Did the send succeed?
     */
    bool SendData(const char *data, signed long long length = -1);
    /**
     * @brief ReadData
     * Recieve data using the protocol.
     * @param socket The socket to read from.
     * @param timeoutMsec The
     * @return The data read. (Returns NULL if there was a problem.)
     */
    char* ReadData(int timeoutMsec = 15000);


};

#endif // THREADEDCLIENT_H
