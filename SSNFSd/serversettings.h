#ifndef SERVERCONFIG_H
#define SERVERCONFIG_H

#include <QObject>
#include <QSqlDatabase>
#include <log.h>

class SettingInfo
{
private:
    QString m_name;
    QString m_value;
    QString m_desc;
    QDateTime m_updtTmStmp;
    QString m_updtUser;
    bool m_isNull = true;
public:
    SettingInfo() {}
    SettingInfo(QString name, QString value, QString desc, QDateTime updtTmStmp, QString updtUser)
        : m_name(name), m_value(value), m_desc(desc), m_updtTmStmp(updtTmStmp), m_updtUser(updtUser) { m_isNull = false; }

    const QString name() { return m_name; }
    const QString value() { return m_value; }
    const QString description() { return m_desc; }
    const QDateTime updtTmStmp() { return m_updtTmStmp; }
    const QString updtUser() { return m_updtUser; }
    bool isNull() { return m_isNull; }
};

class ServerSettings : public QObject
{
    Q_OBJECT
public:
    static const QString get(QString key) { SettingInfo ifo = m_settings.value(key);
                                            return ifo.value(); }
    static const SettingInfo getInfo(QString key) { return m_settings.value(key); }
    static const QString set(QString key, QString value, QString updtUser);
    //static const QHash<QString, SettingInfo> settings() { return m_settings; }

    static void reloadSettings();

private:
    static QHash<QString, SettingInfo> m_settings;
};

//extern ServerSettings settings;

#endif // SERVERCONFIG_H
