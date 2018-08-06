#ifndef INITIFACE_H
#define INITIFACE_H

#include <QObject>

class InitIface : public QObject
{
    Q_OBJECT
public:
    explicit InitIface(QObject *parent = nullptr);

signals:

public slots:
};

#endif // INITIFACE_H