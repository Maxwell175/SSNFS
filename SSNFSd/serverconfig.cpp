#include "serverconfig.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlField>

ServerConfig::ServerConfig(QSqlDatabase myDB, QObject *parent) : QObject(parent) , configDB(myDB)
{ }

void ServerConfig::reloadConfig() {
    QSqlQuery query("SELECT `Setting_Key`, `Setting_Value` FROM `Settings`", configDB);

    if (query.exec()) {
        while (query.next()) {
            QSqlRecord currRecord = query.record();
            QString settingName = currRecord.field(0).value().toString();
            QString settingValue = currRecord.field(1).value().toString();
            if (settingName == "PrivateKeySource") {
                m_privateKeySource = settingValue;
            } else if (settingName == "PrivateKeyFilePath") {
                m_privateKeyFilePath = settingValue;
            } else if (settingName == "CertificatePath") {
                m_certificatePath = settingValue;
            }
        }
    } else {

    }
}
