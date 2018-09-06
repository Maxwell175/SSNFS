/*
 * SSNFS Client v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2018 Maxwell Dreytser
 */

#include "fuseclient.h"

#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <QDir>
#include <QTimer>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include <string.h>

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)
#define HELLO_STR "SSNFS client version " STR(_CLIENT_VERSION)

const int maxRetryCount = 3;
const int retryDelay = 5;
const int timeouts = -1;

FuseClient::FuseClient(QObject *parent) : QObject(parent)
{
    connect(&myThread, SIGNAL(started()), this, SLOT(started()), Qt::QueuedConnection);
    connect(&myThread, &QThread::finished, []() {
        qApp->quit();
    });
    myThread.start();
    moveToThread(&myThread);
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
static int fs_call_release(const char *path, struct fuse_file_info *fi) {
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_release(path, fi);
}
static int fs_call_statfs(const char *path, struct statvfs *stbuf) {
    void *fuseClnt = fuse_get_context()->private_data;
    return ((FuseClient*)(fuseClnt))->fs_statfs(path, stbuf);
}
static void fs_call_destroy(void *private_data) {
    ((FuseClient*)(private_data))->fs_destroy();
}

static int fs_call_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs) {
    return ((FuseClient*)(data))->fs_opt_proc(data, arg, key, outargs);
}
int FuseClient::fs_opt_proc(void *data, const char *arg, int key, fuse_args *outargs) {
    (void) outargs; (void) data;

    switch (key) {
    case FUSE_OPT_KEY_OPT:
        /* Pass through */
        return 1;

    case FUSE_OPT_KEY_NONOPT:
    {
        QString strArg(arg);
        if (host.isNull() && shareName.isNull() && strArg.count(QLatin1Char('/')) == 1) {
            QStringList parts = strArg.split('/');
            QStringList hostParts = parts[0].split(':');
            host = hostParts[0];
            bool portOK = true;
            port = hostParts.length() >= 2 ? ((QString)hostParts[1]).toUShort(&portOK) : 2050;
            if (!portOK) {
                qCritical() << "Invalid server port specified.";
                exit(1);
            }

            shareName = parts[1];
            return 0;
        }
        else if (mountPath.isNull()) {
            struct stat buf;
            stat(arg, &buf);
            // If the directory is inaccessible specifically due to a FUSE error code, try unmounting this broken mount.
            if (errno == ENOTCONN) {
                QString fuseUnmount("fusermount -u ");
                fuseUnmount.append(strArg);
                system(ToChr(fuseUnmount));
            }
            QFileInfo mountPathInfo(strArg);
            if (mountPathInfo.exists() == false || !mountPathInfo.isDir() || mountPathInfo.isSymLink()) {
                qCritical() << "The specified mount path does not exist or is not a valid directory.";
                return -1;
            } else if (QDir(strArg).isEmpty() == false) {
                qCritical() << "The specified mount directory is not empty.";
                return -1;
            }
            mountPath = strArg;
            return 0;
        }
        qCritical() << "Invalid argument:" << arg;
        return -1;
}
    default:
        qCritical() << "Internal error while parsing arguments.";
        abort();
    }
}

void FuseClient::started() {

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
    fs_oper.release = fs_call_release;
    fs_oper.statfs = fs_call_statfs;
    fs_oper.destroy = fs_call_destroy;

    /*int argc = 6;
    char *argv[6];
    argv[0] = QCoreApplication::instance()->arguments().at(0).toUtf8().data();
    argv[1] = "-f";
    argv[2] = "-s";
    argv[3] = "-o";
    argv[4] = "allow_other,direct_io";
    argv[5] = mountPath.toUtf8().data();*/

    char *argv[qApp->arguments().length()];
    for (int i = 0; i < qApp->arguments().length(); i++) {
        QString arg = qApp->arguments().at(i);
        argv[i] = new char[arg.length() + 1];
        strcpy(argv[i], arg.toUtf8().constData());
        argv[i][arg.length()] = '\0';
    }

    struct fuse_args args = FUSE_ARGS_INIT(qApp->arguments().length(), argv);
    struct fuse *fuse;
    struct fuse_session *se;
    struct fuse_chan *ch;
    int res = -1;

    struct fuse_opt myfs_opts[3];
    myfs_opts[0] = FUSE_OPT("certPath=%s", certPath, 0);
    myfs_opts[1] = FUSE_OPT("privKeyPath=%s", privKeyPath, 0);
    myfs_opts[2] = FUSE_OPT_END;

    if (fuse_opt_parse(&args, this, myfs_opts, &fs_call_opt_proc) == -1)
        exit(1);

    if (host.isNull() || shareName.isNull() || mountPath.isNull()) {
        qCritical() << "Not enough arguments. You must at least include <server>/<shareName> and <mountPath>";
        exit(1);
    }

    // Make sure initial connection works.
    if (initSocket() != 0) {
        qInfo() << "Could not establish a connection to the server:" << socket->errorString();
        exit(1);
    }

    syncLocalUsers();

    ch = fuse_mount(mountPath.toUtf8().data(), &args);
    if (!ch) {
        qCritical() << "Error initializing the mount.";
        exit(1);
    }

    fuse = fuse_new(ch, &args, &fs_oper,
                sizeof(struct fuse_operations), this);
    if (!fuse) {
        qCritical() << "Error setting up fuse.";
        exit(1);
    }
    se = fuse_get_session(fuse);
    res = fuse_set_signal_handlers(se);
    if (res != 0) {
        fuse_destroy(fuse);
        exit(1);
    }

    fuse_loop(fuse);

    fuse_remove_signal_handlers(se);
    fuse_unmount(mountPath.toUtf8().data(), ch);
    fuse_destroy(fuse);

    myThread.quit();
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
        socket->setPrivateKey(_CONFIG_DIR "/key.pem");
        socket->setLocalCertificate(_CONFIG_DIR "/cert.pem");

        socket->connectToHostEncrypted(host, port);

        if (socket->waitForEncrypted(timeouts) == false) {
            retryCounter++;
            QThread::currentThread()->sleep(retryDelay);
            continue;
        }

        socket->write(Common::getBytes(Common::Hello));
        socket->write(Common::getBytes((qint32)strlen(HELLO_STR)));
        socket->write(HELLO_STR);
        QByteArray SysInfo = Common::getSystemInfo();
        socket->write(Common::getBytes((qint32)SysInfo.length()));
        socket->write(SysInfo);
        if (socket->waitForBytesWritten(timeouts) == false) {
            retryCounter++;
            QThread::currentThread()->sleep(retryDelay);
            continue;
        }

        if (socket->waitForReadyRead(timeouts) == false) {
            retryCounter++;
            QThread::currentThread()->sleep(retryDelay);
            continue;
        }
        Common::ResultCode result = Common::getResultFromBytes(Common::readExactBytes(socket, 1));
        switch (result) {
        case Common::Hello:
            break;
        case Common::Error:
        {
            qint32 serverHelloLen = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
            QByteArray serverHello = Common::readExactBytes(socket, serverHelloLen);
            qCritical() << "Error connecting to server:" << serverHello;

            retryCounter++;
            QThread::currentThread()->sleep(retryDelay);
            continue;
        }
            break;
        default:
            if (socket->isOpen()) {
                socket->close();
                if (socket->waitForDisconnected(timeouts / 10) == false) {
                    retryCounter++;
                    QThread::currentThread()->sleep(retryDelay);
                    continue;
                }
            }

            retryCounter++;
            QThread::currentThread()->sleep(retryDelay);
            continue;

            break;
        }

        qint32 serverHelloLen = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
        QByteArray serverHello = Common::readExactBytes(socket, serverHelloLen);

        if (serverHello.isNull()) {
            if (socket->isOpen()) {
                socket->close();
                socket->waitForDisconnected(timeouts / 10);
            }

            retryCounter++;
            QThread::currentThread()->sleep(retryDelay);
            continue;
        }

        QString helloStr(serverHello);
        // TODO: Log this?

        socket->write(Common::getBytes(Common::Share));
        socket->write(Common::getBytes((qint32)shareName.toUtf8().length()));
        socket->write(shareName.toUtf8());
        if (socket->waitForBytesWritten(timeouts) == false) {
            retryCounter++;
            QThread::currentThread()->sleep(retryDelay);
            continue;
        }

        if (socket->waitForReadyRead(timeouts) == false) {
            retryCounter++;
            QThread::currentThread()->sleep(retryDelay);
            continue;
        }
        Common::ResultCode shareResult = Common::getResultFromBytes(Common::readExactBytes(socket, 1));
        switch (shareResult) {
        case Common::OK:
            break;
        case Common::Error:
        {
            qint32 serverHelloLen = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
            QByteArray serverHello = Common::readExactBytes(socket, serverHelloLen);
            qCritical() << "Error connecting to server:" << serverHello;

            retryCounter++;
            QThread::currentThread()->sleep(retryDelay);
            continue;
        }
            break;
        default:
            if (socket->isOpen()) {
                socket->close();
                if (socket->waitForDisconnected(timeouts / 10) == false) {
                    retryCounter++;
                    QThread::currentThread()->sleep(retryDelay);
                    continue;
                }
            }

            retryCounter++;
            QThread::currentThread()->sleep(retryDelay);
            continue;

            break;
        }

        return 0;
    }

    return -1;
}

void FuseClient::syncLocalUsers() {
    Common::ResultCode res;

    if (initSocket() == -1) {
        return;
    }

    socket->write(Common::getBytes(Common::UpdateLocalUsers));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during UpdateLocalUsers";
        return;
    }

    QFile passwdRead("/etc/passwd");
    if (passwdRead.open(QFile::ReadOnly) == false) {
        qWarning() << "Unable to open local user account file for reading.";
        return;
    }

    passwdRead.waitForReadyRead(-1);
    while (!passwdRead.atEnd()) {
        QString userLine = passwdRead.readLine();
        QList<QString> lineParts = userLine.split(':');
        QByteArray usernameBytes = lineParts[0].toUtf8();
        bool uidOK = false;
        quint32 uid = lineParts[2].toInt(&uidOK);
        if (uidOK) {
            socket->write(Common::getBytes((qint32)usernameBytes.length()));
            socket->write(usernameBytes);
            socket->write(Common::getBytes(uid));
        }
    }
    socket->write(Common::getBytes((qint32)-1));

    passwdRead.close();

    socket->waitForBytesWritten(-1);
    socket->waitForReadyRead(-1);

    res = Common::getResultFromBytes(Common::readExactBytes(socket, 1));

    Q_ASSERT(res == Common::OK);

    passwdWatcher.addPath("/etc/passwd");
}

void *FuseClient::fs_init(struct fuse_conn_info *conn)
{
    (void) conn;
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

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    socket->write(Common::getBytes((quint32)strlen(path)));
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
    (void) offset;
    (void) fi;

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

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    socket->write(Common::getBytes((quint16)strlen(path)));
    socket->write(path);
    socket->waitForBytesWritten(-1);

    int entryCount = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    for (int i = 0; i < entryCount; i++) {
        qint32 entryLen = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
        QByteArray entry = Common::readExactBytes(socket, entryLen);

        if (filler(buf, entry.data(), NULL, 0))
            break;
    }

    qDebug() << "readdir " << path << ": Done" << timer.elapsed();

    return 0;
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

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    socket->write(Common::getBytes((quint16)strlen(path)));
    socket->write(path);

    socket->write(Common::getBytes((qint32)fi->flags));
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

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    socket->write(Common::getBytes((quint16)strlen(path)));
    socket->write(path);

    qDebug() << "read " << path << ": Sending data:" << timer.elapsed();

    socket->write(Common::getBytes(fd));

    socket->write(Common::getBytes((qint64)size));

    socket->write(Common::getBytes((qint64)offset));

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

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    socket->write(Common::getBytes((quint16)strlen(path)));
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

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    socket->write(Common::getBytes((quint16)strlen(path)));
    socket->write(path);

    socket->write(Common::getBytes((qint32)size));
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

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    socket->write(Common::getBytes((quint16)strlen(path)));
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

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    socket->write(Common::getBytes((quint16)strlen(path)));
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

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    socket->write(Common::getBytes((quint16)strlen(path)));
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

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    socket->write(Common::getBytes((quint16)strlen(path)));
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

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    QDir mountDir(mountPath);
    QByteArray relativeFrom = mountDir.relativeFilePath(from).toUtf8();
    socket->write(Common::getBytes((quint16)relativeFrom.length()));
    socket->write(relativeFrom);
    socket->write(Common::getBytes((quint16)strlen(to)));
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

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    socket->write(Common::getBytes((quint16)strlen(from)));
    socket->write(from);
    socket->write(Common::getBytes((quint16)strlen(to)));
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

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    socket->write(Common::getBytes((quint16)strlen(path)));
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

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    socket->write(Common::getBytes((quint16)strlen(path)));
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

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    socket->write(Common::getBytes((quint16)strlen(path)));
    socket->write(path);

    socket->write(Common::getBytes((quint32)size));
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

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    socket->write(Common::getBytes((quint16)strlen(path)));
    socket->write(path);

    socket->write(Common::getBytes((qint64)ts[0].tv_sec));
    socket->write(Common::getBytes((qint64)ts[0].tv_nsec));

    socket->write(Common::getBytes((qint64)ts[1].tv_sec));
    socket->write(Common::getBytes((qint64)ts[1].tv_nsec));
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
    if (clientWriteBuffer.FdsUsed.contains(fd) == false) {
        clientWriteBuffer.FdsUsed.push_back(fd);

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

        socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

        socket->write(Common::getBytes((quint16)strlen(path)));
        socket->write(path);

        qDebug() << "write " << path << ": Sending data:" << timer.elapsed();

        socket->write(Common::getBytes(fd));

        socket->write(Common::getBytes((qint64)size));

        socket->write(Common::getBytes((qint64)offset));

        //socket->waitForBytesWritten(-1);

        QByteArray bytesToWrite(buf, size);
        /*int written = 0;
    while (written < size) {
        qint32 sizeToWrite = (4000 > size - written) ? size - written : 4000;
        socket->write(Common::getBytes((qint32)sizeToWrite));
        socket->write(bytesToWrite.mid(written, sizeToWrite));
        socket->waitForBytesWritten(-1);
        socket->waitForReadyRead(-1);
        qDebug() << socket->readAll();
        written += sizeToWrite;
        qDebug() << "write " << path << ": Written batch:" << timer.elapsed();
    }

    socket->write(Common::getBytes((qint32) -1));*/
        socket->write(bytesToWrite);
        while (socket->bytesToWrite() > 0)
            socket->waitForBytesWritten(-1);
        socket->flush();

        qDebug() << "write " << path << ": Waiting for reply:" << timer.elapsed();
        res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
        qDebug() << "write " << path << ": Got first reply:" << timer.elapsed();

        if (res < 0) {
            qWarning() << "Warning! bad return code!";
        }

        qDebug() << "write " << path << ":" << res <<"bytes Done:" << timer.elapsed();

        return res;
    } else {
        // Attempt to optimize things a little by checking if this write is consecutive to the last.
        if (clientWriteBuffer.length() > 0 && clientWriteBuffer.last().fd == fd && clientWriteBuffer.last().offset + clientWriteBuffer.last().size == offset) {
            // If so, combine them.
            clientWriteBuffer.last().data.append(buf, size);
            clientWriteBuffer.last().size += size;
        } else {
            WriteRequest req;
            req.fd = fd;
            req.path = path;
            req.size = (qint64)size;
            req.offset = (qint64)offset;
            req.data.append(buf, size);
            clientWriteBuffer.append(req);
        }
        clientWriteBuffer.bytesInBatch += size;

        uint bytesPerBatch = 1048576; // 1MB

        if (clientWriteBuffer.bytesInBatch > bytesPerBatch) {
            int result = writeBuffer();

            return result == 0 ? size : result;
        } else {
            return size;
        }
    }
}

qint32 FuseClient::writeBuffer()
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

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    socket->write(Common::getBytes((quint32)clientWriteBuffer.length()));

    for (int i = 0; i < clientWriteBuffer.length(); i++) {

        socket->flush();

        socket->write(Common::getBytes((quint16)clientWriteBuffer[i].path.length()));
        socket->write(clientWriteBuffer[i].path.toUtf8());

        socket->write(Common::getBytes(clientWriteBuffer[i].fd));

        socket->write(Common::getBytes(clientWriteBuffer[i].size));

        socket->write(Common::getBytes(clientWriteBuffer[i].offset));

        socket->write(clientWriteBuffer[i].data);
    }
    socket->waitForBytesWritten(-1);
    socket->waitForReadyRead(-1);

    qint32 result = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    clientWriteBuffer.clear();
    clientWriteBuffer.bytesInBatch = 0;

    return result;
}

int FuseClient::fs_release(const char *path, fuse_file_info *fi)
{
    QTime timer;
    timer.start();

    int res = writeBuffer();

    if (initSocket() == -1) {
        return -ECOMM;
    }

    // TODO: Log the HELLO msg.
    qDebug() << "release " << path << ": Ended handshake:" << timer.elapsed();

    socket->write(Common::getBytes(Common::release));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during release" << path;
        return -ECOMM;
    }

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    socket->write(Common::getBytes((quint16)strlen(path)));
    socket->write(path);

    socket->write(Common::getBytes((qint32)fi->fh));
    socket->waitForBytesWritten(-1);

    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    qDebug() << "release " << path << ": Done" << timer.elapsed();

    return res;
}

int FuseClient::fs_statfs(const char *path, fs_statvfs *stbuf)
{
    QTime timer;
    timer.start();

    if (initSocket() == -1) {
        return -ECOMM;
    }

    // TODO: Log the HELLO msg.
    qDebug() << "statfs " << path << ": Ended handshake:" << timer.elapsed();

    int res;

    socket->write(Common::getBytes(Common::statfs));
    socket->waitForBytesWritten(-1);

    if (Common::getResultFromBytes(Common::readExactBytes(socket, 1)) != Common::OK) {
        qDebug() << "Error during statfs" << path;
        return -ECOMM;
    }

    socket->write(Common::getBytes((qint32)fuse_get_context()->uid));

    socket->write(Common::getBytes((quint16)strlen(path)));
    socket->write(path);

    QByteArray bufBytes = Common::readExactBytes(socket, sizeof(struct statvfs));
    memcpy(stbuf, bufBytes.data(), bufBytes.length());

    res = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

    qDebug() << "statfs " << path << ": Done" << timer.elapsed();

    return res;
}

void FuseClient::fs_destroy()
{
    writeBuffer();
}
