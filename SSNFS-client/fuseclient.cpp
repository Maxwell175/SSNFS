/*
 * SSNFS Client v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2017 Maxwell Dreytser
 */

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
#include <errno.h>

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)
#define HELLO_STR "SSNFS client version " STR(_CLIENT_VERSION)

const int maxRetryCount = 3;
const int retryDelay = 10;

// For now the values are hardcoded. TODO: Config.
FuseClient::FuseClient(QObject *parent) : QObject(parent),
    mountPath("/home/maxwell/fuse-test-mount"),
    caCertPath("/home/maxwell/CLionProjects/SSNFS/SSNFSd/ca.crt")
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
static int fs_call_access(const char *path, int mask)
{
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_access(path, mask);
}
static int fs_call_readlink(const char *path, char *buf, size_t size)
{
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_readlink(path, buf, size);
}
static int fs_call_mknod(const char *path, mode_t mode, dev_t rdev)
{
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_mknod(path, mode, rdev);
}

void FuseClient::started()
{
    struct fuse_operations fs_oper = {};

    fs_oper.init = fs_call_init;
    fs_oper.getattr = fs_call_getattr;
    fs_oper.readdir = fs_call_readdir;
    fs_oper.open = fs_call_open;
    fs_oper.read = fs_call_read;
    fs_oper.access = fs_call_access;
    fs_oper.readlink = fs_call_readlink;
    fs_oper.mknod = fs_call_mknod;

    int argc = 4;
    char *argv[4];
    argv[0] = QCoreApplication::instance()->arguments().at(0).toUtf8().data();
    argv[1] = "-f";
    argv[2] = "-s";
    argv[3] = mountPath.toUtf8().data();

    fuse_main(argc, argv, &fs_oper, this);
}

int FuseClient::initSocket()
{
    socket->waitForDisconnected(0);
    if (socket->isOpen() && socket->state() == QSslSocket::ConnectedState)
        return 0;

    int retryCounter = 0;
    while (retryCounter < maxRetryCount) {

        socket->deleteLater();

        socket = new QSslSocket(this);

        socket->setCaCertificates(QSslSocket::systemCaCertificates());
        socket->setPeerVerifyMode(QSslSocket::VerifyNone);
        socket->addCaCertificates(caCertPath);

        socket->connectToHostEncrypted("localhost", 2050);

        if (socket->waitForEncrypted(-1) == false) {
            retryCounter++;
            QThread::currentThread()->sleep(retryDelay);
            continue;
        }

        socket->write(Common::getBytes(Common::Hello));
        socket->write(Common::getBytes((int32_t)strlen(HELLO_STR)));
        socket->write(HELLO_STR);
        socket->waitForBytesWritten(-1);

        socket->waitForReadyRead(-1);
        if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::Hello) {
            if (socket->isOpen()) {
                socket->close();
                socket->waitForDisconnected(-1);
            }

            retryCounter++;
            QThread::currentThread()->sleep(retryDelay);
            continue;
        }
        int32_t serverHelloLen = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
        QByteArray serverHello = Common::readExactBytes(socket, serverHelloLen);

        if (serverHello.isNull()) {
            if (socket->isOpen()) {
                socket->close();
                socket->waitForDisconnected(-1);
            }

            retryCounter++;
            QThread::currentThread()->sleep(retryDelay);
            continue;
        }

        QString helloStr(serverHello);
        // TODO: Log this?

        return 0;
    }

    return -1;
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

    if (initSocket() == -1) {
        return -ECOMM;
    }

    // TODO: Log the HELLO msg.
    qDebug() << "getattr " << path << ": Ended handshake:" << timer.elapsed();

    socket->write(Common::getBytes(Common::getattr));
    socket->waitForBytesWritten(-1);

    Common::ResultCode result = Common::getResultFromBytes(Common::readExactBytes(socket, 1));
    if (result != Common::OK) {
        qDebug() << "Error during getattr" << path;
        return -ECOMM;
    }

    socket->write(Common::getBytes((uint16_t)strlen(path)));
    socket->write(path);
    socket->waitForBytesWritten(-1);

    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    QByteArray statData = Common::readExactBytes(socket, sizeof(struct stat));

    memcpy(stbuf, statData.data(), statData.length());

    qDebug() << "getattr " << path << ": Done" << timer.elapsed();

    return res;
}

int FuseClient::fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                           off_t offset, struct fuse_file_info *fi)
{
    QTime timer;
    timer.start();

    int res;

    if (initSocket() == -1) {
        return -ECOMM;
    }

    // TODO: Log the HELLO msg.
    qDebug() << "readdir " << path << ": Ended handshake:" << timer.elapsed();

    socket->write(Common::getBytes(Common::readdir));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during readdir" << path;
        return -ECOMM;
    }

    socket->write(Common::getBytes((uint16_t)strlen(path)));
    socket->write(path);
    socket->waitForBytesWritten(-1);

    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
    if (res != 0) {
        return res;
    }

    int direntCount = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    QByteArray reply = Common::readExactBytes(socket, sizeof(struct dirent) * direntCount);

    struct dirent *dirents = (struct dirent*)reply.data();

    for (int i = 0; i < direntCount; i++) {
        struct dirent de = dirents[i];

        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de.d_ino;
        st.st_mode = de.d_type << 12;
        if (filler(buf, de.d_name, &st, 0))
            break;
    }

    qDebug() << "readdir " << path << ": Done" << timer.elapsed();

    return res;
}

int FuseClient::fs_open(const char *path, struct fuse_file_info *fi)
{
    QTime timer;
    timer.start();

    int res;

    if (initSocket() == -1) {
        return -ECOMM;
    }

    // TODO: Log the HELLO msg.
    qDebug() << "open " << path << ": Ended handshake:" << timer.elapsed();

    socket->write(Common::getBytes(Common::open));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during open" << path;
        return -ECOMM;
    }

    socket->write(Common::getBytes((uint16_t)strlen(path)));
    socket->write(path);

    socket->write(Common::getBytes(fi->flags));
    socket->waitForBytesWritten(-1);

    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    if (res > 0)
        fi->fh = res;

    qDebug() << "open " << path << ": Done" << timer.elapsed();

    return res > 0 ? 0 : res;
}

int FuseClient::fs_read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi)
{
    QTime timer;
    timer.start();

    int fd;
    int res;

    if (fi == NULL) {
        struct fuse_file_info openfi;
        openfi.flags = O_RDONLY;
        fs_open(path, &openfi);
        fd = openfi.fh;
    } else
        fd = fi->fh;

    if (fd == -1)
        return -errno;

    qDebug() << "read " << path << ": Before init:" << timer.elapsed();

    if (initSocket() == -1) {
        return -ECOMM;
    }

    qDebug() << "read " << path << ": Ended handshake:" << timer.elapsed();

    socket->write(Common::getBytes(Common::read));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during read" << path;
        return -ECOMM;
    }

    socket->write(Common::getBytes((uint16_t)strlen(path)));
    socket->write(path);

    qDebug() << "read " << path << ": Sending data:" << timer.elapsed();

    socket->write(Common::getBytes(fd));

    socket->write(Common::getBytes((int32_t)size));

    socket->write(Common::getBytes((int32_t)offset));

    qDebug() << "read " << path << ": Waiting for reply:" << timer.elapsed();
    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
    qDebug() << "read " << path << ": Got first reply:" << timer.elapsed();

    if (res > 0) {
        QByteArray reply = Common::readExactBytes(socket, res);
        memcpy(buf, reply.data(), reply.length());
    }

    qDebug() << "read " << path << ":" << res <<"bytes Done:" << timer.elapsed();

    return res;
}

int FuseClient::fs_access(const char *path, int mask)
{
    QTime timer;
    timer.start();

    int res;

    if (initSocket() == -1) {
        return -ECOMM;
    }

    // TODO: Log the HELLO msg.
    qDebug() << "access " << path << ": Ended handshake:" << timer.elapsed();

    socket->write(Common::getBytes(Common::access));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during access" << path;
        return -ECOMM;
    }

    socket->write(Common::getBytes((uint16_t)strlen(path)));
    socket->write(path);

    socket->write(Common::getBytes(mask));
    socket->waitForBytesWritten(-1);

    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    qDebug() << "access " << path << ": Done" << timer.elapsed();

    return res;
}

int FuseClient::fs_readlink(const char *path, char *buf, size_t size)
{
    QTime timer;
    timer.start();

    int res;

    if (initSocket() == -1) {
        return -ECOMM;
    }

    // TODO: Log the HELLO msg.
    qDebug() << "readlink " << path << ": Ended handshake:" << timer.elapsed();

    socket->write(Common::getBytes(Common::readlink));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during readlink" << path;
        return -ECOMM;
    }

    socket->write(Common::getBytes((uint16_t)strlen(path)));
    socket->write(path);

    socket->write(Common::getBytes((int32_t)size));
    socket->waitForBytesWritten(-1);

    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    if (res > 0) {
        QByteArray outbuf = Common::readExactBytes(socket, res);
        memcpy(buf, outbuf.data(), res);
        buf[res] = '\0';
    }

    qDebug() << "readlink " << path << ": Done" << timer.elapsed();

    return 0;
}

int FuseClient::fs_mknod(const char *path, mode_t mode, dev_t rdev)
{
    if (S_ISBLK(mode) || S_ISCHR(mode)) {
        return -EINVAL;
    }
    (void) rdev;

    QTime timer;
    timer.start();

    int res;

    if (initSocket() == -1) {
        return -ECOMM;
    }

    // TODO: Log the HELLO msg.
    qDebug() << "mknod " << path << ": Ended handshake:" << timer.elapsed();

    socket->write(Common::getBytes(Common::mknod));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during mknod" << path;
        return -ECOMM;
    }

    socket->write(Common::getBytes((uint16_t)strlen(path)));
    socket->write(path);

    socket->write(Common::getBytes((int32_t)mode));
    socket->waitForBytesWritten(-1);

    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    qDebug() << "mknod " << path << ": Done" << timer.elapsed();

    if (res < 0)
        return res;
    else
        return 0;
}
