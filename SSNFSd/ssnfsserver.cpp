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
#include <QDir>

#include <limits>
#include <errno.h>

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)
#define HELLO_STR "SSNFS server version " STR(_SERVER_VERSION)

// For now the values are hardcoded. TODO: Config.

SSNFSServer::SSNFSServer(QObject *parent)
    : QTcpServer(parent), testBase("/home/maxwell/fuse-test-base"), // This is the directory that we will serve up.
      privateKeyPath("/home/maxwell/CLionProjects/SSNFS/SSNFSd/test.key"),
      certPath("/home/maxwell/CLionProjects/SSNFS/SSNFSd/test.crt"),
      caCertPath("/home/maxwell/CLionProjects/SSNFS/SSNFSd/ca.crt")
{

}

void SSNFSServer::incomingConnection(qintptr socketDescriptor)
{
    QSslSocket *socket = new QSslSocket();

    socket->setPeerVerifyMode(QSslSocket::VerifyNone);
    socket->addCaCertificates(caCertPath);
    socket->setLocalCertificate(certPath);
    socket->setPrivateKey(privateKeyPath);
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

void SSNFSServer::ReadyToRead(SSNFSClient *sender)
{
    QSslSocket *socket = sender->socket;

    switch (sender->status) {
    case WaitingForHello:
    {
        if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::Hello) {
            if (socket->isOpen()) {
                socket->close();
            }
            return;
        }
        int32_t serverHelloLen = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
        QByteArray serverHello = Common::readExactBytes(socket, serverHelloLen);

        if (serverHello.isNull()) {
            if (socket->isOpen()) {
                socket->close();
            }
            return;
        }

        QString helloStr(serverHello);
        // TODO: Log this?

        socket->write(Common::getBytes(Common::Hello));
        socket->write(Common::getBytes((int32_t)strlen(HELLO_STR)));
        socket->write(HELLO_STR);

        sender->status = WaitingForOperation;
    }
        break;
    case WaitingForOperation:
    {
        Common::Operation currOperation = Common::getOperationFromBytes(Common::readExactBytes(socket, 2));

        if (currOperation == Common::InvalidOperation) {
            qDebug() << "Error! Invalid operation from client.";
            socket->close();
            return;
        }

        socket->write(Common::getBytes(Common::OK));

        sender->status = InOperation;
        sender->operation = currOperation;
        sender->operationStep = 1;
    }
        break;
    case InOperation:
    {
        if (sender->operation == Common::getattr) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                // Paths need to be int16 because in Win 10 long paths can be enabled.
                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                struct stat stbuf;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                res = lstat(finalPath.toUtf8().data(), &stbuf);

                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                QByteArray statData;
                statData.fill('\x00', sizeof(struct stat));
                memcpy(statData.data(), &stbuf, sizeof(struct stat));

                //socket->write((char*)(&statData), sizeof(stbuf));
                socket->write(statData);

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::readdir) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                DIR *dp;
                struct dirent *de;

                dp = opendir(finalPath.toUtf8().data());
                if (dp == NULL) {
                    res = -errno;

                    socket->write(Common::getBytes(res));

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

                socket->write(Common::getBytes(res));

                socket->write(Common::getBytes(dirents.length()));

                socket->write((char*)(&outDirents), sizeof(outDirents));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::open) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                int oflags = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

                res = open(finalPath.toUtf8().data(), oflags);
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

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::read) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                sender->timer.restart();

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                QString finalPath(testBase);
                finalPath.append(targetPath);

                // Get the internal File Descriptor.
                int fakeFd = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

                int readSize = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

                int readOffset = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

                int res;

                void *readBuf = malloc(readSize);
                int actuallyRead = -1;

                // Find the corresponding actual File Descriptor
                int realFd = sender->fds.value(fakeFd);

                // We will now use the proc virtual filesystem to look up info about the File Descriptor.
                QString procFdPath = tr("/proc/self/fd/%1").arg(realFd);

                struct stat procFdInfo;

                lstat(procFdPath.toUtf8().data(), &procFdInfo);

                QVector<char> actualFdFilePath;
                actualFdFilePath.fill('\x00', procFdInfo.st_size + 1);

                ssize_t r = readlink(procFdPath.toUtf8().data(), actualFdFilePath.data(), procFdInfo.st_size + 1);

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
                        if (finalPath == actualFdFilePath.data()) {
                            actuallyRead = pread(realFd, readBuf, readSize, readOffset);
                            res = actuallyRead;
                        } else {
                            res = -EBADF;
                        }
                    }
                }

                socket->write(Common::getBytes(res));

                if (res > 0)
                    socket->write((char*)(readBuf), actuallyRead);

                sender->operationData.clear();
                sender->operationStep = 1;

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::access) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                int mask = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

                // TODO: Add additional restrictions to prevent problems across users.
                res = access(finalPath.toUtf8().data(), mask);

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::readlink) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                int size = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

                QByteArray buf;
                buf.fill('\0', size);

                // TODO: Add additional restrictions to prevent problems across users.
                res = readlink(finalPath.toUtf8().data(), buf.data(), size - 1);

                if (res == -1)
                    socket->write(Common::getBytes(-errno));
                else {
                    // Convert paths to be relative to base.
                    QDir base(testBase);
                    QString result = base.relativeFilePath(buf.data());

                    socket->write(Common::getBytes(result.length()));

                    socket->write(result.toUtf8().data());
                }


                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::mknod) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                unsigned int mode = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

                /* On Linux this could just be 'mknod(path, mode, rdev)' but this
                   is more portable */
                if (S_ISFIFO(mode))
                    res = mkfifo(finalPath.toUtf8().data(), mode);
                else
                    res = mknod(finalPath.toUtf8().data(), mode, (dev_t) 0);
                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::mkdir) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                unsigned int mode = Common::getUInt32FromBytes(Common::readExactBytes(socket, 4));

                res = mkdir(finalPath.toUtf8().data(), mode);

                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::unlink) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                res = unlink(finalPath.toUtf8().data());

                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::rmdir) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                res = rmdir(finalPath.toUtf8().data());

                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::symlink) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t oldPathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray oldPath = Common::readExactBytes(socket, oldPathLength);

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                res = symlink(oldPath.data(), finalPath.toUtf8().data());

                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::rename) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t FromPathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray FromPath = Common::readExactBytes(socket, FromPathLength);

                uint16_t ToPathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray ToPath = Common::readExactBytes(socket, ToPathLength);

                int res;

                QString finalFromPath(testBase);
                finalFromPath.append(FromPath);
                QString finalToPath(testBase);
                finalToPath.append(ToPath);

                res = rename(finalFromPath.toUtf8().data(), finalToPath.toUtf8().data());

                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::chmod) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                unsigned int mode = Common::getUInt32FromBytes(Common::readExactBytes(socket, 4));

                // TODO: This is just a temporary version. Really, this should be much more complex.
                res = chmod(finalPath.toUtf8().data(), mode);

                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::chown) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                unsigned int uid = Common::getUInt32FromBytes(Common::readExactBytes(socket, 4));
                unsigned int gid = Common::getUInt32FromBytes(Common::readExactBytes(socket, 4));

                // TODO: This is just a temporary version. Really, this should be much more complex.
                res = lchown(finalPath.toUtf8().data(), uid, gid);

                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

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
