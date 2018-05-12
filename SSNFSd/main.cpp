/*
 * SSNFS Client v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2017 Maxwell Dreytser
 */

#include <QCoreApplication>
#include <QFileInfo>
#include <QSslKey>

#include <log.h>
#include <serversettings.h>
#include <ssnfsserver.h>

QHash<QString, LogCategory> Log::Categories;
QVector<LogOutput> Log::Outputs;
bool Log::isInit = false;
QHash<QString, SettingInfo> ServerSettings::m_settings;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    ServerSettings::reloadSettings();

    if (app.arguments().contains("-h", Qt::CaseInsensitive) || app.arguments().contains("--help", Qt::CaseInsensitive)) {
        qInfo() << QFileInfo(app.applicationFilePath()).fileName() << "[OPTIONS]";
        qInfo() << "    --help, -h                  Show this help text.";
        qInfo() << "    --set-pkey-file=<path>      Set server private key to the specified file.";
        qInfo() << "    --set-cert-file=<path>      Set server certificate to the specified file.";
        qInfo() << "    --hash-password-salt=<salt> When manually hasing a password (below) use this salt.";
        qInfo() << "                            Must be specified before --hash-password.";
        qInfo() << "    --hash-password=<password>  Return a hashed version of the specified password.";
        qInfo() << "Note: Will exit after setting the private key or certificate, or manually generating a hashed password.";
        return 0;
    }
    bool willExit = false;
    QString manualSalt;
    foreach (QString arg, app.arguments()) {
        if (arg.startsWith("--set-pkey-file", Qt::CaseInsensitive)) {
            int equPos;
            if ((equPos = arg.indexOf('=')) == -1) {
                qInfo() << "Invalid parameter:" << arg;
            }
            QString newFile = arg.mid(equPos + 1);
            QFile pkeyFile(newFile);
            if (pkeyFile.exists() == false) {
                qInfo() << "The file you specified for --set-pkey-file does not exist.";
            } else {
                if (pkeyFile.open(QFile::ReadOnly) == false) {
                    qInfo() << "Could not open the file specified in --set-pkey-file:" << pkeyFile.errorString();
                } else {
                    QSslKey pkey(pkeyFile.readAll(), QSsl::Rsa);
                    pkeyFile.close();
                    if (pkey.isNull()) {
                        qInfo() << "The file you specified for --set-pkey-file is not a valid PEM-encoded RSA private key.";
                    } else {
                        ServerSettings::set("PrivateKeySource", "file", "Console");
                        ServerSettings::set("PrivateKeyFilePath", newFile, "Console");
                        qInfo() << "New private key file has been set successfully.";
                    }
                }
            }
            willExit = true;
        } else if (arg.startsWith("--set-cert-file", Qt::CaseInsensitive)) {
            int equPos;
            if ((equPos = arg.indexOf('=')) == -1) {
                qInfo() << "Invalid parameter:" << arg;
            }
            QString newFile = arg.mid(equPos + 1);
            QFile certFile(newFile);
            if (certFile.exists() == false) {
                qInfo() << "The file you specified for --set-cert-file does not exist.";
            } else {
                if (certFile.open(QFile::ReadOnly) == false) {
                    qInfo() << "Could not open the file specified in --set-cert-file:" << certFile.errorString();
                } else {
                    QSslCertificate cert(certFile.readAll());
                    certFile.close();
                    if (cert.isNull()) {
                        qInfo() << "The file you specified for --set-cert-file is not a valid PEM-encoded certificate.";
                    } else {
                        ServerSettings::set("PrivateKeySource", newFile, "Console");
                        qInfo() << "New private key file has been set successfully.";
                    }
                }
            }
            willExit = true;
        } else if (arg.startsWith("--hash-password-salt", Qt::CaseInsensitive)) {
            int equPos;
            if ((equPos = arg.indexOf('=')) == -1) {
                qInfo() << "Invalid parameter:" << arg;
            }
            manualSalt = arg.mid(equPos + 1);
        } else if (arg.startsWith("--hash-password", Qt::CaseInsensitive)) {
            int equPos;
            if ((equPos = arg.indexOf('=')) == -1) {
                qInfo() << "Invalid parameter:" << arg;
            }
            QString inputPass = arg.mid(equPos + 1);

            QByteArray shaPass = QCryptographicHash::hash(inputPass.toUtf8(), QCryptographicHash::Sha512);
            qInfo() << Common::GetPasswordHash(shaPass.toHex().toLower(), manualSalt);
            qInfo() << shaPass.toHex().toLower();

            willExit = true;
        }
    }

    if (willExit)
        return 0;


    qInfo() << "Starting SSNFSd" << _SERVER_VERSION << "at" << QDateTime::currentDateTime().toString();

    Log::init();

    SSNFSServer server;

    bool settingOk = false;
    int dbPort = ServerSettings::get("ListenPort").toInt(&settingOk);
    if (!settingOk && dbPort > 0 && dbPort < UINT16_MAX) {
        Log::error(Log::Categories["Core"], "Listen port in config DB is not a valid port number.");
        exit(1);
    }

    server.listen(QHostAddress::Any, dbPort);

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
                Log::error(Log::Categories["Core"], "Cannot open config database: {0}", configDB.lastError().text().toUtf8().data());
            } else {
                qCritical() << "Error opening config DB:" << configDB.lastError().text();
            }
            return QSqlDatabase();
        }
        QSqlQuery enableForeignKeys("PRAGMA foreign_keys = \"1\"", configDB);
        if (enableForeignKeys.exec() == false) {
            if (Log::isInit) {
                Log::error(Log::Categories["Core"], "Error while turning on foreign keys on config database: {0}", configDB.lastError().text().toUtf8().data());
            } else {
                qCritical() << "Error while turning on foreign keys on config DB:" << configDB.lastError().text();
            }
            return QSqlDatabase();
        }
        QSqlQuery foreignKeyCheck("PRAGMA foreign_key_check");
        if (foreignKeyCheck.exec() == false) {
            if (Log::isInit) {
                Log::error(Log::Categories["Core"], "Error while running foreign key checks on config database: {0}", configDB.lastError().text().toUtf8().data());
            } else {
                qCritical() << "Error while running Foreign Key checks on config DB:" << configDB.lastError().text();
            }
            return QSqlDatabase();
        }
        if (foreignKeyCheck.next()) {
            qInfo() << foreignKeyCheck.size();
            if (Log::isInit) {
                Log::error(Log::Categories["Core"], "One or more Foreign Key violations have been detected in the config DB! Unable to load config.");
            } else {
                // A FK violation can only be made by user interaction with the DB. So, they should fix their errors.
                qCritical() << "One or more Foreign Key violations have been detected in the config DB.";
            }
            return QSqlDatabase();
        }
    }

    return configDB;
}
