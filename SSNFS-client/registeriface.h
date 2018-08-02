#ifndef REGISTERIFACE_H
#define REGISTERIFACE_H

#include <QObject>
#include <QSslSocket>

#include <common.h>

class RegisterIface : public QObject
{
    Q_OBJECT
public:
    explicit RegisterIface(QObject *parent = nullptr);

private:
    QSslSocket socket;

signals:

public slots:
};

#endif // REGISTERIFACE_H
