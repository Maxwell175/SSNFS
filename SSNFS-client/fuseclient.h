/*
 * SSNFS Client v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2018 Maxwell Dreytser
 */

#ifndef FUSECLIENT_H
#define FUSECLIENT_H

#include <QObject>
#include <QThread>
#include <fuse.h>
#include <QSslSocket>
#include <QFileSystemWatcher>
#include <common.h>

#define BUFFER_PER_FILE 10240

typedef struct stat fs_stat;
typedef struct statvfs fs_statvfs;

struct FuseOpts {
    char *certPath;
    char *privKeyPath;
};
#define FUSE_OPT(t, p, v) { t, offsetof(struct FuseOpts, p), v }

class WriteRequest
{
public:
    int fd;
    QString path;
    qint64 size;
    qint64 offset;
    QByteArray data;
};

class WriteRequestBatch : public QVector<WriteRequest>
{
public:
    quint64 bytesInBatch = 0;

    QVector<int> FdsUsed;
};

class FuseClient : public QObject
{
    Q_OBJECT
public:
    explicit FuseClient(QObject *parent = nullptr);

private:
    QString mountPath;
    QString host;
    quint16 port;
    QString shareName;
    struct FuseOpts opts;

    QThread myThread;

    QSslSocket *socket = new QSslSocket(this);

    QFileSystemWatcher passwdWatcher;

    QByteArray ReadData(int timeoutMsec = -1);
    bool SendData(const char *data, signed long long length = -1);

    int initSocket();

    WriteRequestBatch clientWriteBuffer;

    qint32 writeBuffer();

signals:

private slots:
    void started();
    void syncLocalUsers();

public slots:
    // FUSE callbacks
    int fs_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs);

    void *fs_init(struct fuse_conn_info *conn);
    int fs_getattr(const char *path, fs_stat *stbuf);
    int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi);
    int fs_open(const char *path, struct fuse_file_info *fi);
    int fs_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi);
    int fs_access(const char *path, int mask);
    int fs_readlink(const char *path, char *buf, size_t size);
    int fs_mknod(const char *path, mode_t mode, dev_t rdev);
    int fs_mkdir(const char *path, mode_t mode);
    int fs_unlink(const char *path);
    int fs_rmdir(const char *path);
    int fs_symlink(const char *from, const char *to);
    int fs_rename(const char *from, const char *to);
    int fs_chmod(const char *path, mode_t mode);
    int fs_chown(const char *path, uid_t uid, gid_t gid);
    int fs_truncate(const char *path, off_t size);
    int fs_utimens(const char *path, const struct timespec ts[2]);
    int fs_write(const char *path, const char *buf, size_t size,
                        off_t offset, struct fuse_file_info *fi);
    int fs_release(const char *path, struct fuse_file_info *fi);
    int fs_statfs(const char *path, fs_statvfs *stbuf);
    void fs_destroy();
    // FUSE callbacks

};

#endif // FUSECLIENT_H
