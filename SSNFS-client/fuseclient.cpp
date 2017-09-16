#include "fuseclient.h"

#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

#include "threadedclient.h"
#include "clientmanager.h"

const char *testBase = "/home/maxwell/fuse-test-base";

FuseClient::FuseClient(QObject *parent) : QObject(parent)
{
    moveToThread(&myThread);
    connect(&myThread, SIGNAL(started()), this, SLOT(started()), Qt::QueuedConnection);
    myThread.start();
}

// These functions just call the class functions below. This is done to allow event loop stuff to be done.
static void *fs_call_init(struct fuse_conn_info *conn)
{
    void *fuseClnt = fuse_get_context()->private_data;
    ((FuseClient*)(fuseClnt))->fs_init(conn);
    return fuseClnt;
}
static int fs_call_getattr(const char *path, struct stat *stbuf)
{
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_getattr(path, stbuf);
}
static int fs_call_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                           off_t offset, struct fuse_file_info *fi)
{
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_readdir(path, buf, filler, offset, fi);
}
static int fs_call_open(const char *path, struct fuse_file_info *fi)
{
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_open(path, fi);
}
static int fs_call_read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi)
{
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_read(path, buf, size, offset, fi);
}

void FuseClient::started()
{
    struct fuse_operations fs_oper = {};

    fs_oper.init = fs_call_init;
    fs_oper.getattr = fs_call_getattr;
    fs_oper.readdir = fs_call_readdir;
    fs_oper.open = fs_call_open;
    fs_oper.read = fs_call_read;

    int argc = 4;
    char *argv[4];
    argv[0] = QCoreApplication::instance()->arguments().at(0).toUtf8().data();
    argv[1] = "-f";
    argv[2] = "-s";
    argv[3] = "/home/maxwell/fuse-test-mount";

    fuse_main(argc, argv, &fs_oper, this);
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
    length  = ((unsigned long int)(quint8)lengthBytes[0]) << 24;
    length |= ((unsigned long int)(quint8)lengthBytes[1]) << 16;
    length |= ((unsigned long int)(quint8)lengthBytes[2]) << 8;
    length |= ((unsigned long int)(quint8)lengthBytes[3]);

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

void *FuseClient::fs_init(struct fuse_conn_info *conn)
{
    // Currently we do nothing. We might want to do something at some point.
    // Return value is ignored.
    // TODO: Remove it?
    return NULL;
}

int FuseClient::fs_getattr(const char *path, fs_stat *stbuf)
{
    QTime timer;
    timer.start();

    int res;

    QSslSocket *socket = new QSslSocket(this);
    socket->setCaCertificates(QSslSocket::systemCaCertificates());
    socket->setPeerVerifyMode(QSslSocket::VerifyNone);
    socket->addCaCertificates("/home/maxwell/CLionProjects/SSNFS/SSNFSd/ca.crt");

    socket->connectToHostEncrypted("localhost", 2050);

    socket->waitForEncrypted(-1);

    SendData(socket, tr("HELLO: SSNFS client version %1").arg(_CLIENT_VERSION).toUtf8().data());
    QByteArray serverHello = ReadData(socket);

    if (serverHello.isNull()) {
        if (socket->isOpen()) {
            socket->close();
            socket->waitForDisconnected(-1);
        }
        socket->deleteLater();

        return -1;
    }

    QString helloStr(serverHello);

    if (!helloStr.startsWith("HELLO: ")) {
        SendData(socket, "ERROR: Invalid response to HELLO!");
        if (socket->isOpen()) {
            socket->close();
            socket->waitForDisconnected(-1);
        }
        socket->deleteLater();

        return -1;
    }
    // TODO: Log the HELLO msg.
    qDebug() << "Ended handshake:" << timer.elapsed();

    SendData(socket, "OPERATION: getaddr");

    QByteArray reply;

    reply = ReadData(socket);
    if (reply.isNull()) {
        // TODO: Log the error.
        return -1;
    }
    if (tr(reply) != "OPERATION OK") {
        // TODO: Log the error.
        return -1;
    }

    SendData(socket, path);

    reply = ReadData(socket);
    if (reply.isNull()) {
        // TODO: Log the error.
        return -1;
    }
    if (tr(reply).startsWith("RETURN VALUE: ") == false) {
        // TODO: Log the error.
        return -1;
    }

    res = tr(reply).split(": ").at(1).toInt();

    reply = ReadData(socket);
    if (reply.isNull()) {
        // TODO: Log the error.
        return -1;
    }
    if (tr(reply) != "RETURN PARAM: stbuf") {
        // TODO: Log the error.
        return -1;
    }

    QByteArray statData = ReadData(socket);

    memcpy(stbuf, statData.data(), statData.length());

    qDebug() << "Closing" << timer.elapsed();

    socket->close();
    socket->waitForDisconnected(-1);

    socket->deleteLater();

    qDebug() << "Done" << timer.elapsed();

    return res;
}

int FuseClient::fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                           off_t offset, struct fuse_file_info *fi)
{
    QString actualPath(testBase);
    actualPath.append(path);

    qDebug() << actualPath;

    DIR *dp;
    struct dirent *de;

    dp = opendir(actualPath.toUtf8().data());
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }

    closedir(dp);
    return 0;
}

int FuseClient::fs_open(const char *path, struct fuse_file_info *fi)
{
    QString actualPath(testBase);
    actualPath.append(path);

    qDebug() << actualPath;

    int res;

    res = open(actualPath.toUtf8().data(), fi->flags);
    if (res == -1)
        return -errno;

    fi->fh = res;
    return 0;
}

int FuseClient::fs_read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi)
{
    QString actualPath(testBase);
    actualPath.append(path);

    qDebug() << actualPath;

    int fd;
    int res;

    if(fi == NULL)
        fd = open(actualPath.toUtf8().data(), O_RDONLY);
    else
        fd = fi->fh;

    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    if(fi == NULL)
        close(fd);
    return res;
}
