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
#include <QDir>
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
static int fs_call_mkdir(const char *path, mode_t mode)
{
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_mkdir(path, mode);
}
static int fs_call_unlink(const char *path)
{
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_unlink(path);
}
static int fs_call_rmdir(const char *path)
{
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_rmdir(path);
}
static int fs_call_symlink(const char *from, const char *to)
{
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_symlink(from, to);
}
static int fs_call_rename(const char *from, const char *to)
{
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_rename(from, to);
}
static int fs_call_chmod(const char *path, mode_t mode)
{
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_chmod(path, mode);
}
static int fs_call_chown(const char *path, uid_t uid, gid_t gid)
{
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_chown(path, uid, gid);
}
static int fs_call_truncate(const char *path, off_t size)
{
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_truncate(path, size);
}
static int fs_call_utimens(const char *path, const timespec ts[2])
{
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_utimens(path, ts);
}
static int fs_call_write(const char *path, const char *buf, size_t size,
                         off_t offset, struct fuse_file_info *fi) {
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_write(path, buf, size, offset, fi);
}
static int fs_call_flush(const char *path, struct fuse_file_info *fi) {
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_flush(path, fi);
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
    fs_oper.mkdir = fs_call_mkdir;
    fs_oper.unlink = fs_call_unlink;
    fs_oper.rmdir = fs_call_rmdir;
    fs_oper.symlink = fs_call_symlink;
    fs_oper.rename = fs_call_rename;
    fs_oper.chmod = fs_call_chmod;
    fs_oper.chown = fs_call_chown;
    fs_oper.truncate = fs_call_truncate;
    fs_oper.utimens = fs_call_utimens;
    fs_oper.write = fs_call_write;
    fs_oper.flush = fs_call_flush;

    int argc = 6;
    char *argv[6];
    argv[0] = QCoreApplication::instance()->arguments().at(0).toUtf8().data();
    argv[1] = "-f";
    argv[2] = "-s";
    argv[3] = "-o";
    argv[4] = "allow_other,direct_io";
    argv[5] = mountPath.toUtf8().data();

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

    socket->write(Common::getBytes((int64_t)size));

    socket->write(Common::getBytes((int64_t)offset));

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

    socket->write(Common::getBytes(mode));
    socket->waitForBytesWritten(-1);

    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    qDebug() << "mknod " << path << ": Done" << timer.elapsed();

    if (res < 0)
        return res;
    else
        return 0;
}

int FuseClient::fs_mkdir(const char *path, mode_t mode)
{
    QTime timer;
    timer.start();

    int res;

    if (initSocket() == -1) {
        return -ECOMM;
    }

    // TODO: Log the HELLO msg.
    qDebug() << "mkdir " << path << ": Ended handshake:" << timer.elapsed();

    socket->write(Common::getBytes(Common::mkdir));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during mkdir" << path;
        return -ECOMM;
    }

    socket->write(Common::getBytes((uint16_t)strlen(path)));
    socket->write(path);

    socket->write(Common::getBytes(mode));
    socket->waitForBytesWritten(-1);

    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    qDebug() << "mkdir " << path << ": Done" << timer.elapsed();

    return res;
}

int FuseClient::fs_unlink(const char *path)
{
    QTime timer;
    timer.start();

    int res;

    if (initSocket() == -1) {
        return -ECOMM;
    }

    // TODO: Log the HELLO msg.
    qDebug() << "unlink " << path << ": Ended handshake:" << timer.elapsed();

    socket->write(Common::getBytes(Common::unlink));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during unlink" << path;
        return -ECOMM;
    }

    socket->write(Common::getBytes((uint16_t)strlen(path)));
    socket->write(path);

    socket->waitForBytesWritten(-1);

    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    qDebug() << "unlink " << path << ": Done" << timer.elapsed();

    return res;
}

int FuseClient::fs_rmdir(const char *path)
{
    QTime timer;
    timer.start();

    int res;

    if (initSocket() == -1) {
        return -ECOMM;
    }

    // TODO: Log the HELLO msg.
    qDebug() << "rmdir " << path << ": Ended handshake:" << timer.elapsed();

    socket->write(Common::getBytes(Common::rmdir));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during rmdir" << path;
        return -ECOMM;
    }

    socket->write(Common::getBytes((uint16_t)strlen(path)));
    socket->write(path);

    socket->waitForBytesWritten(-1);

    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    qDebug() << "rmdir " << path << ": Done" << timer.elapsed();

    return res;
}

int FuseClient::fs_symlink(const char *from, const char *to)
{
    QTime timer;
    timer.start();

    int res;

    if (initSocket() == -1) {
        return -ECOMM;
    }

    // TODO: Log the HELLO msg.
    qDebug() << "symlink " << to << ": Ended handshake:" << timer.elapsed();

    socket->write(Common::getBytes(Common::symlink));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during symlink" << from;
        return -ECOMM;
    }

    QDir mountDir(mountPath);
    QByteArray relativeFrom = mountDir.relativeFilePath(from).toUtf8();
    socket->write(Common::getBytes((uint16_t)relativeFrom.length()));
    socket->write(relativeFrom);
    socket->write(Common::getBytes((uint16_t)strlen(to)));
    socket->write(to);

    socket->waitForBytesWritten(-1);

    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    qDebug() << "symlink " << to << ": Done" << timer.elapsed();

    return res;
}

int FuseClient::fs_rename(const char *from, const char *to)
{
    QTime timer;
    timer.start();

    int res;

    if (initSocket() == -1) {
        return -ECOMM;
    }

    // TODO: Log the HELLO msg.
    qDebug() << "rename " << from << ": Ended handshake:" << timer.elapsed();

    socket->write(Common::getBytes(Common::rename));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during rename" << from;
        return -ECOMM;
    }

    socket->write(Common::getBytes((uint16_t)strlen(from)));
    socket->write(from);
    socket->write(Common::getBytes((uint16_t)strlen(to)));
    socket->write(to);

    socket->waitForBytesWritten(-1);

    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    qDebug() << "rename " << from << ": Done" << timer.elapsed();

    return res;
}

int FuseClient::fs_chmod(const char *path, mode_t mode)
{
    QTime timer;
    timer.start();

    int res;

    if (initSocket() == -1) {
        return -ECOMM;
    }

    // TODO: Log the HELLO msg.
    qDebug() << "chmod " << path << ": Ended handshake:" << timer.elapsed();

    socket->write(Common::getBytes(Common::chmod));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during chmod" << path;
        return -ECOMM;
    }

    socket->write(Common::getBytes((uint16_t)strlen(path)));
    socket->write(path);

    socket->write(Common::getBytes(mode));
    socket->waitForBytesWritten(-1);

    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    qDebug() << "chmod " << path << ": Done" << timer.elapsed();

    return res;
}

int FuseClient::fs_chown(const char *path, uid_t uid, gid_t gid)
{
    QTime timer;
    timer.start();

    int res;

    if (initSocket() == -1) {
        return -ECOMM;
    }

    // TODO: Log the HELLO msg.
    qDebug() << "chown " << path << ": Ended handshake:" << timer.elapsed();

    socket->write(Common::getBytes(Common::chown));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during chown" << path;
        return -ECOMM;
    }

    socket->write(Common::getBytes((uint16_t)strlen(path)));
    socket->write(path);

    socket->write(Common::getBytes(uid));
    socket->write(Common::getBytes(gid));
    socket->waitForBytesWritten(-1);

    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    qDebug() << "chown " << path << ": Done" << timer.elapsed();

    return res;
}

int FuseClient::fs_truncate(const char *path, off_t size)
{
    QTime timer;
    timer.start();

    int res;

    if (initSocket() == -1) {
        return -ECOMM;
    }

    // TODO: Log the HELLO msg.
    qDebug() << "truncate " << path << ": Ended handshake:" << timer.elapsed();

    socket->write(Common::getBytes(Common::truncate));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during truncate" << path;
        return -ECOMM;
    }

    socket->write(Common::getBytes((uint16_t)strlen(path)));
    socket->write(path);

    socket->write(Common::getBytes((uint32_t)size));
    socket->waitForBytesWritten(-1);

    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    qDebug() << "truncate " << path << ": Done" << timer.elapsed();

    return res;
}

int FuseClient::fs_utimens(const char *path, const timespec ts[2])
{
    QTime timer;
    timer.start();

    int res;

    if (initSocket() == -1) {
        return -ECOMM;
    }

    // TODO: Log the HELLO msg.
    qDebug() << "utimens " << path << ": Ended handshake:" << timer.elapsed();

    socket->write(Common::getBytes(Common::utimens));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during utimens" << path;
        return -ECOMM;
    }

    socket->write(Common::getBytes((uint16_t)strlen(path)));
    socket->write(path);

    socket->write(Common::getBytes((int64_t)ts[0].tv_sec));
    socket->write(Common::getBytes((int64_t)ts[0].tv_nsec));

    socket->write(Common::getBytes((int64_t)ts[1].tv_sec));
    socket->write(Common::getBytes((int64_t)ts[1].tv_nsec));
    socket->waitForBytesWritten(-1);

    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    qDebug() << "utimens " << path << ": Done" << timer.elapsed();

    return res;
}

int FuseClient::fs_write(const char *path, const char *buf, size_t size,
                         off_t offset, struct fuse_file_info *fi) {
    QTime timer;
    timer.start();

    int fd;
    int res;

    if (fi == NULL) {
        struct fuse_file_info openfi;
        openfi.flags = O_WRONLY;
        fs_open(path, &openfi);
        fd = openfi.fh;
    } else
        fd = fi->fh;

    if (fd == -1)
        return -errno;

    // Run first request directly to check for errors.
    if (fdBuffers.contains(fd) == false) {
        fdBuffers.insert(fd, WriteRequestBatch());

        qDebug() << "write " << path << ": Before init:" << timer.elapsed();

        if (initSocket() == -1) {
            return -ECOMM;
        }

        qDebug() << "write " << path << ": Ended handshake:" << timer.elapsed();

        socket->write(Common::getBytes(Common::write));
        socket->waitForBytesWritten(-1);

        if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
            qDebug() << "Error during write" << path;
            return -ECOMM;
        }

        socket->write(Common::getBytes((uint16_t)strlen(path)));
        socket->write(path);

        qDebug() << "write " << path << ": Sending data:" << timer.elapsed();

        socket->write(Common::getBytes(fd));

        socket->write(Common::getBytes((int64_t)size));

        socket->write(Common::getBytes((int64_t)offset));

        //socket->waitForBytesWritten(-1);

        QByteArray bytesToWrite(buf, size);
        /*int written = 0;
    while (written < size) {
        int32_t sizeToWrite = (4000 > size - written) ? size - written : 4000;
        socket->write(Common::getBytes((int32_t)sizeToWrite));
        socket->write(bytesToWrite.mid(written, sizeToWrite));
        socket->waitForBytesWritten(-1);
        socket->waitForReadyRead(-1);
        qDebug() << socket->readAll();
        written += sizeToWrite;
        qDebug() << "write " << path << ": Written batch:" << timer.elapsed();
    }

    socket->write(Common::getBytes((int32_t) -1));*/
        socket->write(bytesToWrite);
        while (socket->bytesToWrite() > 0)
            socket->waitForBytesWritten(-1);

        qDebug() << "write " << path << ": Waiting for reply:" << timer.elapsed();
        res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
        qDebug() << "write " << path << ": Got first reply:" << timer.elapsed();

        if (res < 0) {
            qWarning() << "Warning! bad return code!";
        }

        qDebug() << "write " << path << ":" << res <<"bytes Done:" << timer.elapsed();

        return res;
    } else {
        WriteRequest req;
        req.fd = fd;
        req.path = path;
        req.size = (int64_t)size;
        req.offset = (int64_t)offset;
        req.data.resize(size);
        std::copy(buf, buf+size, req.data.begin());
        fdBuffers[fd].append(req);
        fdBuffers[fd].bytesInBatch += size;

        int bytesPerBatch = 10240;
        if (offset > 50 * 1000000) {
            bytesPerBatch = 102400;
        }

        if (fdBuffers[fd].bytesInBatch > bytesPerBatch) {
            int result = writeBuffer(fd);

            return result == 0 ? size : result;
        } else {
            return size;
        }
    }
}

int32_t FuseClient::writeBuffer(int fd)
{
    if (initSocket() == -1) {
        return -ECOMM;
    }

    socket->write(Common::getBytes(Common::writebulk));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during writebulk";
        return -ECOMM;
    }

    socket->write(Common::getBytes((uint32_t)fdBuffers[fd].length()));

    for (int i = 0; i < fdBuffers[fd].length(); i++) {
        socket->write(Common::getBytes((uint16_t)fdBuffers[fd][i].path.length()));
        socket->write(fdBuffers[fd][i].path.toUtf8());

        socket->write(Common::getBytes(fdBuffers[fd][i].fd));

        socket->write(Common::getBytes(fdBuffers[fd][i].size));

        socket->write(Common::getBytes(fdBuffers[fd][i].offset));

        socket->write(fdBuffers[fd][i].data);
    }
    qDebug() << socket->bytesToWrite();
    socket->waitForBytesWritten(-1);
    socket->waitForReadyRead(-1);

    int32_t result = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    fdBuffers[fd].clear();
    fdBuffers[fd].bytesInBatch = 0;

    return result;
}

int FuseClient::fs_flush(const char *path, fuse_file_info *fi)
{
    return writeBuffer(fi->fh);
    // TODO: add call to `close(dup(fi->fh)` on server! See: https://github.com/libfuse/libfuse/blob/master/example/passthrough_fh.c#L466
}
