#include "ssnfsserver.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <stdio.h>

#include <QEventLoop>

SSNFSServer::SSNFSServer(QObject *parent)
    : QTcpServer(parent), testBase("/home/maxwell/fuse-test-base")
{

}

void SSNFSServer::incomingConnection(qintptr socketDescriptor)
{
    QSslSocket *socket = new QSslSocket();

    socket->setPeerVerifyMode(QSslSocket::VerifyNone);
    socket->addCaCertificates("/home/maxwell/CLionProjects/SSNFS/SSNFSd/ca.crt");
    socket->setLocalCertificate("/home/maxwell/CLionProjects/SSNFS/SSNFSd/test.crt");
    socket->setPrivateKey("/home/maxwell/CLionProjects/SSNFS/SSNFSd/test.key");
    //socket->setProtocol(QSsl::TlsV1SslV3);

    if (!socket->setSocketDescriptor(socketDescriptor)) {
        // TODO: Log socket.error()
        qDebug() << socket->errorString();
        qDebug() << socket->sslErrors();
        return;
    }

    socket->startServerEncryption();

    SSNFSClient *currClient = new SSNFSClient();
    currClient->socket = socket;
    currClient->status = WaitingForHello;
    clients.append(currClient);

    connect(socket, static_cast<void(QSslSocket::*)(const QList<QSslError> &)>(&QSslSocket::sslErrors),
            [this, currClient](QList<QSslError> errors) {
        this->sslErrorsOccurred(currClient, errors);
    });
    connect(socket, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
            [this, currClient](QAbstractSocket::SocketError socketError) {
        this->socketError(currClient, socketError);
    });
    connect(socket, &QSslSocket::disconnected, [this, currClient]() {
        this->socketDisconnected(currClient);
    });
    connect(socket, &QSslSocket::readyRead, [this, currClient]() {
        this->ReadyToRead(currClient);
    });
}

void SSNFSServer::sslErrorsOccurred(SSNFSClient *sender, const QList<QSslError> &errors)
{
    (void) sender;
    // TODO: Log client data.
    qDebug() << errors;
}

void SSNFSServer::socketError(SSNFSClient *sender, QAbstractSocket::SocketError socketError)
{
    if (sender->status != WaitingForHello && sender->status != WaitingForOperation) {
        return;
    }

    qDebug() << sender->socket->errorString();

    if (sender->socket->isOpen()) {
        sender->socket->close();
    } else {
        sender->socket->deleteLater();

        clients.removeAll(sender);

        delete sender;
    }
}

void SSNFSServer::socketDisconnected(SSNFSClient *sender)
{
    qDebug() << "Socket has disconnected.";

    sender->socket->deleteLater();

    clients.removeAll(sender);

    delete sender;
}


bool SendData(QSslSocket *socket, const char *data, signed long long length = -1)
{
    // First, get the length of the data.
    unsigned long int inputLength;
    if (length < 0) {
        // + 1 is for the null terminator.
        inputLength = strlen(data);
    } else {
        inputLength = length;
    }

    // Account for 5 extra pad bytes at the end.
    unsigned long int dataLength = inputLength + 5;

    // Convert it into bytes.
    unsigned char lengthBytes[4];
    lengthBytes[0] = (dataLength >> 24) & 0xFF;
    lengthBytes[1] = (dataLength >> 16) & 0xFF;
    lengthBytes[2] = (dataLength >> 8) & 0xFF;
    lengthBytes[3] = dataLength & 0xFF;

    QEventLoop loop;

    // Write the length first.
    socket->write((char*)(&lengthBytes), 4);
    // Then the actual data.
    socket->write(data, inputLength);
    // Finally the pad bytes.
    socket->write("\xFF\xFF\xFF\xFF\xFF", 5);

    // Get the total number of bytes that need to get written.
    unsigned long long totalToWrite = dataLength + 4;

    // Wait for some bytes to be written.
    loop.connect(socket, &QSslSocket::bytesWritten, [&](qint64 byteswritten) {
        // Subtract the bytes written now from the target number of bytes.
        totalToWrite -= byteswritten;

        Q_ASSERT(totalToWrite >= 0);

        // Did we write all the bytes already?
        if (totalToWrite == 0)
            // Seems so. Break out of the QEventLoop.
            loop.quit();
    });
    loop.exec();

    return true;
}
QByteArray ReadData(QSslSocket *socket, int timeoutMsec = -1)
{
    unsigned long int bytesRead = 0;

    char lengthBytes[4] = "";

    QEventLoop lengthLoop;

    auto readLength = [&]() {
        // Read what data has come in.
        char currData[4 - bytesRead];
        int currRead = socket->read((char *)(&currData), 4 - bytesRead);
        for (int i = 0; i < currRead; i++) {
            lengthBytes[bytesRead + i] = currData[i];
        }
        bytesRead += currRead;

        if (bytesRead >= 4) {
            if (lengthLoop.isRunning())
                lengthLoop.quit();
            return true;
        } else {
            return false;
        }
    };

    if (socket->bytesAvailable() > 0) {
        bool gotAllLengthBytes = readLength();
        if (!gotAllLengthBytes) {
            lengthLoop.connect(socket, &QSslSocket::readyRead, readLength);
            // TODO: Add timeout here!!!
            lengthLoop.exec();
        }
    } else {
        lengthLoop.connect(socket, &QSslSocket::readyRead, readLength);
        // TODO: Add timeout here!!!
        lengthLoop.exec();
    }

    // Get a number back from the bytes.
    unsigned long int length = 0;
    length  = ((unsigned long int) lengthBytes[0]) << 24;
    length |= ((unsigned long int) lengthBytes[1]) << 16;
    length |= ((unsigned long int) lengthBytes[2]) << 8;
    length |= ((unsigned long int) lengthBytes[3]);

    // Clear the temp variable.
    bytesRead = 0;

    char data[length] = "";

    QEventLoop dataLoop;

    auto readData = [&]() {
        // Read what data has come in.
        char currData[length - bytesRead];
        int currRead = socket->read((char *)(&currData), length - bytesRead);
        for (int i = 0; i < currRead; i++) {
            data[bytesRead + i] = currData[i];
        }
        bytesRead += currRead;

        if (bytesRead >= length) {
            if (dataLoop.isRunning())
                dataLoop.quit();
            return true;
        } else {
            return false;
        }
    };

    if (socket->bytesAvailable() > 0) {
        bool gotAllLengthBytes = readData();
        if (!gotAllLengthBytes) {
            dataLoop.connect(socket, &QSslSocket::readyRead, readData);
            // TODO: Add timeout here!!!
            dataLoop.exec();
        }
    } else {
        dataLoop.connect(socket, &QSslSocket::readyRead, readData);
        // TODO: Add timeout here!!!
        dataLoop.exec();
    }


    QByteArray output;

    // Copy to output array.
    for (unsigned long int i = 0; i < length; i++) {
        output.append(data[i]);
    }

    // Strip the pad chars aka last 5 bytes.
    output.remove(output.length() - 5, 5);

    return output;
}

void SSNFSServer::ReadyToRead(SSNFSClient *sender)
{
    QSslSocket *socket = sender->socket;

    switch (sender->status) {
    case WaitingForHello:
    {
        QByteArray hello = ReadData(socket);
        if (hello.isNull()) {
            // TODO: Log read error
            if (socket->isOpen())
                socket->close();
            return;
        }

        if (QString(hello).startsWith("HELLO: ") == false) {
            // TODO: Log invalid hello.
            if (socket->isOpen())
                socket->close();
            return;
        }

        if (SendData(socket, QString("HELLO: SSNFS server version %1").arg(_SERVER_VERSION).toUtf8().data()) == false) {
            // TODO: Log write error
            if (socket->isOpen())
                socket->close();
            return;
        }

        sender->status = WaitingForOperation;
    }
        break;
    case WaitingForOperation:
    {
        QByteArray dataRecvd = ReadData(socket);
        if (dataRecvd.isNull()) {
            // TODO: Log read error
            if (socket->isOpen())
                socket->close();
            return;
        }

        QString recvd = QString(dataRecvd);
        QStringList operParts = recvd.split(": ");

        if (operParts[0] != "OPERATION") {
            // TODO: Log no operation.
            //if (socket->isOpen())
            //    socket->close();
            qDebug() << "Expected Operation, got:" << recvd;
            return;
        }

        QStringList availableOperations;
        availableOperations.append("getaddr");

        if (availableOperations.contains(operParts[1])) {
            SendData(socket, "OPERATION OK");

            sender->status = InOperation;
            sender->operation = QString(operParts[1]);
            sender->operationStep = 1;
        }
    }
        break;
    case InOperation:
    {
        if (sender->operation == "getaddr") {
            switch (sender->operationStep) {
            case 1:
            {
                QByteArray targetPath = ReadData(socket);

                int res;

                struct stat stbuf;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                res = lstat(finalPath.toUtf8().data(), &stbuf);

                if (res == -1)
                    res = -errno;

                char resultVal[18] = "";
                snprintf(resultVal, 18, "RETURN VALUE: %d", res);
                SendData(socket, (char*)(&resultVal));

                SendData(socket, "RETURN PARAM: stbuf");

                char statData[sizeof(stbuf)];
                memcpy(&statData, &stbuf, sizeof(stbuf));

                SendData(socket, (char*)(&statData), sizeof(stbuf));
            }
                break;
            }
        }
    }
        break;
    }
}
