#ifndef FUSECLIENT_H
#define FUSECLIENT_H

#include <QObject>
#include <QThread>
#include <fuse.h>

typedef struct stat fs_stat;

class FuseClient : public QObject
{
    Q_OBJECT
public:
    explicit FuseClient(QObject *parent = 0);

private:
    QThread myThread;

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
    // FUSE callbacks

};

#endif // FUSECLIENT_H
