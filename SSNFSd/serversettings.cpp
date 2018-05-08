#include "serversettings.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlField>

void ServerSettings::reloadSettings() {
    QSqlDatabase confDB = getConfDB();
    if (!confDB.isValid()) {
        return;
    }
    QSqlQuery query("SELECT `Setting_Key`, `Setting_Value`, `Setting_Desc`, `Updt_TmStmp`, `Updt_User` FROM `Settings`", confDB);

    if (query.exec()) {
        while (query.next()) {
            QSqlRecord currRecord = query.record();
            QString settingName = currRecord.field(0).value().toString();
            QString settingValue = currRecord.field(1).value().toString();
            QString settingDesc = currRecord.field(2).value().toString();
            QDateTime settingTmStmp = currRecord.field(3).value().toDateTime();
            QString settingUser = currRecord.field(4).value().toString();
            m_settings[settingName] = SettingInfo(settingName, settingValue, settingDesc, settingTmStmp, settingUser);
        }
    } else {
        Log::error(Log::Categories["Core"], "Error retrieving configuration settings from DB: {0}", query.lastError().text().toUtf8().data());
    }
}

const QString ServerSettings::set(QString key, QString value, QString updtUser) {
    QSqlDatabase confDB = getConfDB();
    if (!confDB.isValid()) {
        return QString();
    }
    QSqlQuery query("UPDATE `Settings` SET `Setting_Value` = ?, `Updt_TmStmp` = CURRENT_TIMESTAMP, `Updt_User` = ? WHERE `Setting_Key = ?", confDB);
    query.addBindValue(value);
    query.addBindValue(updtUser);
    query.addBindValue(key);
    if (query.exec() && query.numRowsAffected() == 1) {
        // We need to make a new SettingInfo object since it is impossible to change an existing object.
        m_settings[key] = SettingInfo(key, value, m_settings[key].description(), QDateTime::currentDateTime(), updtUser);
        return value;
    } else {
        Log::error(Log::Categories["Core"], "Error setting configuration setting in DB: {0}", query.lastError().text().toUtf8().data());
        return QString();
    }

    return value;
}
