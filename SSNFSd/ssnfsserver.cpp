/*
 * SSNFS Client v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2017 Maxwell Dreytser
 */

#include "ssnfsserver.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <stdio.h>

#include <QEventLoop>

#include <limits>
#include <errno.h>

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

    //QEventLoop loop;

    // Write the length first.
    socket->write((char*)(&lengthBytes), 4);
    // Then the actual data.
    socket->write(data, inputLength);
    // Finally the pad bytes.
    socket->write("\xFF\xFF\xFF\xFF\xFF", 5);

    // Get the total number of bytes that need to get written.
    unsigned long long totalToWrite = dataLength + 4;

    while (socket->bytesToWrite() != 0) {
        socket->waitForBytesWritten(-1);
    }

    return true;
}
QByteArray ReadData(QSslSocket *socket, int timeoutMsec = -1)
{
    QTime timer;
    timer.start();

    unsigned long int bytesRead = 0;

    char lengthBytes[4];

    while (bytesRead < 4) {
        if (socket->bytesAvailable() == 0)
            socket->waitForReadyRead(1);

        if (socket->bytesAvailable() > 0)
            qDebug() << "got" << socket->bytesAvailable() << "length bytes:" << timer.elapsed();

        char currData[4 - bytesRead];
        int currRead = socket->read((char *)(&currData), 4 - bytesRead);
        for (int i = 0; i < currRead; i++) {
            lengthBytes[bytesRead + i] = currData[i];
        }
        bytesRead += currRead;
    }

    // Get a number back from the bytes.
    unsigned long int length = 0;
    length  = ((unsigned long int)(quint8)lengthBytes[0]) << 24;
    length |= ((unsigned long int)(quint8)lengthBytes[1]) << 16;
    length |= ((unsigned long int)(quint8)lengthBytes[2]) << 8;
    length |= ((unsigned long int)(quint8)lengthBytes[3]);

    // Clear the temp variable.
    bytesRead = 0;

    char data[length];

    while (bytesRead < length) {
        if (socket->bytesAvailable() == 0)
            socket->waitForReadyRead(timeoutMsec);

        char currData[length - bytesRead];
        int currRead = socket->read((char *)(&currData), length - bytesRead);
        for (int i = 0; i < currRead; i++) {
            data[bytesRead + i] = currData[i];
        }
        bytesRead += currRead;
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
        availableOperations.append("getattr");
        availableOperations.append("readdir");
        availableOperations.append("open");
        availableOperations.append("read");

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
        if (sender->operation == "getattr") {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

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

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == "readdir") {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                QByteArray targetPath = ReadData(socket);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                DIR *dp;
                struct dirent *de;

                dp = opendir(finalPath.toUtf8().data());
                if (dp == NULL) {
                    res = -errno;

                    char resultVal[18] = "";
                    snprintf(resultVal, 18, "RETURN VALUE: %d", res);
                    SendData(socket, (char*)(&resultVal));
                    sender->status = WaitingForOperation;
                    return;
                }
                res = 0;

                QVector<struct dirent> dirents;

                while ((de = readdir(dp)) != NULL) {
                    dirents.append(*de);
                }

                struct dirent outDirents[dirents.length()];

                for (int i = 0; i < dirents.length(); i++) {
                    outDirents[i] = dirents[i];
                }

                closedir(dp);

                char resultVal[18] = "";
                snprintf(resultVal, 18, "RETURN VALUE: %d", res);
                SendData(socket, (char*)(&resultVal));

                SendData(socket, QString::number(dirents.length()).toUtf8().data());

                SendData(socket, (char*)(&outDirents), sizeof(outDirents));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == "open") {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                QByteArray targetPath = ReadData(socket);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                SendData(socket, "OK");

                QByteArray flags = ReadData(socket);

                res = open(finalPath.toUtf8().data(), flags.toInt());
                if (res == -1)
                    res = -errno;
                else {
                    int realFd = res;
                    int FakeFd = -1;
                    for (int i = 1; i < std::numeric_limits<int>::max(); i++) {
                        if (sender->fds.keys().contains(i) == false) {
                            FakeFd = i;
                            break;
                        }
                    }
                    if (FakeFd == -1)
                        res = -EBADFD;
                    else {
                        sender->fds.insert(FakeFd, realFd);
                        res = FakeFd;
                    }
                }

                QByteArray result = QByteArray::number(res);
                SendData(socket, result.data(), result.length());

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == "read") {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                sender->timer.restart();

                // Get the file to read.
                QByteArray targetPath = ReadData(socket);

                QString finalPath(testBase);
                finalPath.append(targetPath);

                sender->operationData.insert("path", finalPath);
                sender->operationStep = 2;

                qDebug() << "Done step 1:" << sender->timer.elapsed();
            }
                break;
            case 2:
            {
                qDebug() << "Step 2 start:" << sender->timer.elapsed();

                // Get the internal File Descriptor.
                QByteArray fd = ReadData(socket);
                int fakeFd = fd.toInt();

                sender->operationData.insert("fakeFd", fakeFd);
                sender->operationStep = 3;

                qDebug() << "Done step 2:" << sender->timer.elapsed();
            }
                break;
            case 3:
            {
                int readSize = ReadData(socket).toInt();

                sender->operationData.insert("readSize", readSize);
                sender->operationStep = 4;

                qDebug() << "Done step 3:" << sender->timer.elapsed();
            }
                break;
            case 4:
            {
                QString finalPath = sender->operationData.value("path").toString();
                int fakeFd = sender->operationData.value("fakeFd").toInt();
                int readSize = sender->operationData.value("readSize").toInt();
                int readOffset = ReadData(socket).toInt();

                int res;

                void *readBuf = malloc(readSize);
                int actuallyRead = -1;

                // Find the corresponding actual File Descriptor
                int realFd = sender->fds.value(fakeFd);

                // We will now use the proc virtual filesystem to look up info about the File Descriptor.
                QString procFdPath = tr("/proc/self/fd/%1").arg(realFd);

                struct stat procFdInfo;

                lstat(procFdPath.toUtf8().data(), &procFdInfo);

                char actualFdFilePath[procFdInfo.st_size + 1];

                ssize_t r = readlink(procFdPath.toUtf8().data(), (char*)(&actualFdFilePath), procFdInfo.st_size + 1);

                if (r < 0) {
                    res = -errno;
                } else {
                    actualFdFilePath[procFdInfo.st_size] = '\0';
                    if (r > procFdInfo.st_size) {
                        qWarning() << "The File Descriptor link in /proc increased in size" <<
                                      "between lstat() and readlink()! old size:" << procFdInfo.st_size
                                   << " new size:" << r << " readlink result:" << actualFdFilePath;
                        res = -1;
                    } else {
                        if (finalPath == actualFdFilePath) {
                            actuallyRead = pread(realFd, readBuf, readSize, readOffset);
                            res = actuallyRead;
                        } else {
                            res = -EBADF;
                        }
                    }
                }

                QByteArray result = QByteArray::number(res);
                SendData(socket, result.data(), result.length());

                SendData(socket, (char*)(readBuf), actuallyRead);

                qDebug() << "Done step 4:" << sender->timer.elapsed();

                sender->operationData.clear();
                sender->operationStep = 1;

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        }
    }
        break;
    }
}
