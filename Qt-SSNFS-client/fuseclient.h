/*
 * SSNFS Client v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2017 Maxwell Dreytser
 */

#ifndef FUSECLIENT_H
#define FUSECLIENT_H

#include <QObject>
#include <QThread>
#include <fuse.h>
#include <QSslSocket>
#include <common.h>

typedef struct stat fs_stat;

class FuseClient : public QObject
{
    Q_OBJECT
public:
    explicit FuseClient(QObject *parent = 0);

private:
    QString mountPath;
    QString caCertPath;

    QThread myThread;

    QSslSocket *socket = new QSslSocket(this);

    QByteArray ReadData(int timeoutMsec = -1);
    bool SendData(const char *data, signed long long length = -1);

    int initSocket();

signals:

private slots:
    void started();

public slots:
    // FUSE callbacks
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
    // FUSE callbacks

};

#endif // FUSECLIENT_H
