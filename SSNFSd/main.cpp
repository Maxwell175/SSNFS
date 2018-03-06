/*
 * SSNFS Client v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2017 Maxwell Dreytser
 */

#include <QCoreApplication>

#include "log.h"
#include "ssnfsserver.h"

QHash<QString, LogCategory> Log::Categories;
QVector<LogOutput> Log::Outputs;
bool Log::isInit = false;

int main(int argc, char *argv[])
{
    qInfo() << "Starting SSNFSd" << _SERVER_VERSION << "at" << QDateTime::currentDateTime().toString();

    QCoreApplication app(argc, argv);

    Log::init();

    SSNFSServer server;

    server.listen(QHostAddress::Any, 2050);

    app.exec();
}

QSqlDatabase getConfDB() {
    QSqlDatabase configDB = QSqlDatabase::database(QLatin1String(QSqlDatabase::defaultConnection), false);
    if (!configDB.isValid()) {
        configDB = QSqlDatabase::addDatabase("QSQLITE");
        QString DBPath = QString("%1/config.db").arg(_CONFIG_DIR);
        configDB.setDatabaseName(DBPath);
    }
    if (!configDB.isOpen()) {
        if (configDB.open() == false) {
            if (Log::isInit) {
                Log::error(Log::Categories["Server"], "Cannot open config database: {0}", configDB.lastError().text().toUtf8().data());
                return QSqlDatabase();
            } else {
                qCritical() << "Error opening config DB:" << configDB.lastError().text();
                return QSqlDatabase();
            }
        }
        QSqlQuery enableForeignKeys("PRAGMA foreign_keys = \"1\"", configDB);
        if (enableForeignKeys.exec() == false) {
            if (Log::isInit) {
                Log::error(Log::Categories["Server"], "Error while turning on foreign keys on config database: {0}", configDB.lastError().text().toUtf8().data());
                return QSqlDatabase();
            } else {
                qCritical() << "Error while turning on foreign keys on config DB:" << configDB.lastError().text();
                return QSqlDatabase();
            }
        }
        QSqlQuery foreignKeyCheck("PRAGMA foreign_key_check");
        if (foreignKeyCheck.exec() == false) {
            if (Log::isInit) {
                Log::error(Log::Categories["Server"], "Error while running foreign key checks on config database: {0}", configDB.lastError().text().toUtf8().data());
                return QSqlDatabase();
            } else {
                qCritical() << "Error while running Foreign Key checks on config DB:" << configDB.lastError().text();
                return QSqlDatabase();
            }
        }
        if (foreignKeyCheck.next()) {
            qInfo() << foreignKeyCheck.size();
            if (Log::isInit) {
                Log::error(Log::Categories["Server"], "One or more Foreign Key violations have been detected in the config DB! Unable to load config.");
                return QSqlDatabase();
            } else {
                // A FK violation can only be made by user interaction with the DB. So, they should fix their errors.
                qCritical() << "One or more Foreign Key violations have been detected in the config DB.";
                return QSqlDatabase();
            }
        }
    }

    return configDB;
}
