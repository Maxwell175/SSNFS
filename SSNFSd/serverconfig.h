#ifndef SERVERCONFIG_H
#define SERVERCONFIG_H

#include <QObject>
#include <QSqlDatabase>

class ServerConfig : public QObject
{
    Q_OBJECT
public:
    explicit ServerConfig(QSqlDatabase myDB, QObject *parent = nullptr);

    QString privateKeySource();
    QString privateKeySource(QString newValue);

    QString privateKeyFilePath();
    QString privateKeyFilePath(QString newValue);

    QString certificatePath();
    QString certificatePath(QString newValue);

public slots:
    void reloadConfig();

private:
    QSqlDatabase configDB;

    QString m_privateKeySource;
    QString m_privateKeyFilePath;
    QString m_certificatePath;
};

#endif // SERVERCONFIG_H
