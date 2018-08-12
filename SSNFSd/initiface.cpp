#include "initiface.h"

#include <common.h>
#include <serversettings.h>

#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QDirIterator>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include <openssl/pem.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <openssl/bio.h>

InitIface::InitIface(QObject *parent) : QObject(parent)
{
    QTextStream ConsoleOut(stdout, QIODevice::WriteOnly);
    QTextStream ConsoleIn(stdin, QIODevice::ReadOnly);

    ConsoleOut << "Welcome to the SSNFS server initialization wizard." << endl;
    ConsoleOut << "I will guide you through the setup process." << endl;
    ConsoleOut << "I will ask you a series of questions where you must answer in the specified format." << endl;
    ConsoleOut << "Many questions has default answers specified in [square brakets]. To use these defaults, simply leave your answer blank." << endl;
    ConsoleOut << endl;

    quint16 port = 2050;
    while (true) {
        ConsoleOut << "What port number should the server listen on? [2050] ";
        ConsoleOut.flush();
        QString portEntered = ConsoleIn.readLine();
        if (portEntered.isEmpty()) {
            port = 2050;
            break;
        }
        bool portOK = false;
        port = portEntered.toUShort(&portOK);
        if (portOK) {
            break;
        } else {
            ConsoleOut << "Invalid port number entered." << endl << endl;
        }
    }
    ConsoleOut << endl;

    bool createNewCert = true;
    while (true) {
        ConsoleOut << "Would you like to create a new SSL certificate for the server, or use an existing one? (N)ew/(E)xisting [n] ";
        ConsoleOut.flush();
        QString newCertPrompt = ConsoleIn.readLine();
        if (newCertPrompt.isEmpty() || newCertPrompt.startsWith('n', Qt::CaseInsensitive)) {
            createNewCert = true;
            break;
        } else if (newCertPrompt.startsWith('e', Qt::CaseInsensitive)) {
            createNewCert = false;
            break;
        } else {
            ConsoleOut << "Please enter either N to create a new certificate or E to use an existing one." << endl;
        }
    }

    QString certPath;
    QString keyPath;
    if (createNewCert) {
        while (true) {
            ConsoleOut << "Where would you like the new certificate to be saved? [" _CONFIG_DIR "/cert.pem] ";
            ConsoleOut.flush();
            certPath = ConsoleIn.readLine();
            if (certPath.isEmpty()) certPath = _CONFIG_DIR "/cert.pem";
            QFileInfo newCertInfo(certPath);
            if (newCertInfo.exists()) {
                if (newCertInfo.isDir()) {
                    ConsoleOut << "You have chosen a path to a directory, while a path to a file is required." << endl;
                    continue;
                } else {
                    ConsoleOut << "You have chosen a path to an existing file. Would you like to overwrite it? (Y/N) [n] ";
                    ConsoleOut.flush();
                    QString overwritePrompt = ConsoleIn.readLine();
                    if (overwritePrompt.isEmpty()) overwritePrompt = "n";
                    if (overwritePrompt.toLower() != "y")
                        continue;
                }
            }
            if (newCertInfo.dir().exists() == false) {
                if (newCertInfo.dir().mkpath(".") == false) {
                    ConsoleOut << "Can't create the parent folder(s) in the requested path. Do you have enough privileges?" << endl;
                    continue;
                }
            }
            break;
        }
        while (true) {
            ConsoleOut << "Where would you like the new private key to be saved? [" _CONFIG_DIR "/key.pem] ";
            ConsoleOut.flush();
            keyPath = ConsoleIn.readLine();
            if (keyPath.isEmpty()) keyPath = _CONFIG_DIR "/key.pem";
            QFileInfo newKeyInfo(keyPath);
            if (newKeyInfo.exists()) {
                if (newKeyInfo.isDir()) {
                    ConsoleOut << "You have chosen a path to a directory, while a path to a file is required." << endl;
                    continue;
                } else {
                    ConsoleOut << "You have chosen a path to an existing file. Would you like to overwrite it? (Y/N) [n] ";
                    ConsoleOut.flush();
                    QString overwritePrompt = ConsoleIn.readLine();
                    if (overwritePrompt.isEmpty()) overwritePrompt = "n";
                    if (overwritePrompt.toLower() != "y")
                        continue;
                }
            }
            if (newKeyInfo.dir().exists() == false) {
                if (newKeyInfo.dir().mkpath(".") == false) {
                    ConsoleOut << "Can't create the parent folder(s) in the requested path. Do you have enough privileges?" << endl;
                    continue;
                }
            }
            break;
        }

        ConsoleOut << "Generating a new certificate for this system... ";
        ConsoleOut.flush();

        int ret = 0;
        RSA *rsa = NULL;
        BIGNUM *bne = NULL;
        BIO *pkeywriter = NULL;
        BIO *certwriter = NULL;
        EVP_PKEY * pkey = NULL;

        int bits = 4096;
        unsigned long e = RSA_F4;

        // Generate RSA key.
        bne = BN_new();
        ret = BN_set_word(bne,e);
        if(ret != 1){
            goto free_all;
        }
        rsa = RSA_new();
        ret = RSA_generate_key_ex(rsa, bits, bne, NULL);
        if(ret != 1){
            goto free_all;
        }

        // Write private key.
        pkeywriter = BIO_new_file(ToChr(keyPath), "w");
        ret = PEM_write_bio_RSAPrivateKey(pkeywriter, rsa, NULL, NULL, 0, NULL, NULL);

        // Assign it.
        pkey = EVP_PKEY_new();
        EVP_PKEY_assign_RSA(pkey, rsa);

        // Generate X509.
        X509 * x509;
        x509 = X509_new();
        ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
        X509_gmtime_adj(X509_get_notBefore(x509), 0);
        X509_gmtime_adj(X509_get_notAfter(x509), 60L * 60L * 24L * 365L * 10L); // 10 years.
        X509_set_pubkey(x509, pkey);
        X509_NAME * name;
        name = X509_get_subject_name(x509);
        X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC,
                                   (unsigned char *)"  ", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC,
                                   (unsigned char *)"SSNFS", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "OU",  MBSTRING_ASC,
                                   (unsigned char *)"Server", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                                   (unsigned char *)ToChr(QHostInfo::localHostName()), -1, -1, 0);
        X509_set_issuer_name(x509, name);

        // Sign it.
        X509_sign(x509, pkey, EVP_sha256());

        // Write cert to file.
        certwriter = BIO_new_file(ToChr(certPath), "w");
        PEM_write_bio_X509(certwriter, x509);

        // free
free_all:
        BIO_free_all(pkeywriter);
        BIO_free_all(certwriter);
        RSA_free(rsa);
        BN_free(bne);
        EVP_PKEY_free(pkey);

        if (ret != 1) {
            ConsoleOut << "Failed.\n" << ERR_error_string(ERR_get_error(), NULL) << endl;
            exit(1);
        } else {
            ConsoleOut << "Done" << endl;
        }
    } else {
        while (true) {
            ConsoleOut << "Please enter the path to your existing certificate: ";
            ConsoleOut.flush();
            certPath = ConsoleIn.readLine();
            QFileInfo certInfo(certPath);
            if (certInfo.exists()) {
                if (certInfo.isDir()) {
                    ConsoleOut << "You have chosen a path to a directory, while a path to a file is required." << endl;
                    continue;
                }
            } else {
                ConsoleOut << "The path that you have entered does not point to an existing file." << endl;
                continue;
            }
            break;
        }
        while (true) {
            ConsoleOut << "Please enter the path to your existing private key: ";
            ConsoleOut.flush();
            keyPath = ConsoleIn.readLine();
            QFileInfo keyInfo(keyPath);
            if (keyInfo.exists()) {
                if (keyInfo.isDir()) {
                    ConsoleOut << "You have chosen a path to a directory, while a path to a file is required." << endl;
                    continue;
                }
            } else {
                ConsoleOut << "The path that you have entered does not point to an existing file." << endl;
                continue;
            }
            break;
        }
    }
    ConsoleOut << endl;

    ConsoleOut << "Now we will define the first administrative user that can be later used to create others." << endl << endl;
    QString adminName;
    QString adminEmail;
    QString adminEncPass;
    while (true) {
        ConsoleOut << "Enter your name: ";
        ConsoleOut.flush();
        adminName = ConsoleIn.readLine();
        if (adminName.isEmpty()) {
            ConsoleOut << "Your name cannot be empty." << endl;
            continue;
        }
        break;
    }
    while (true) {
        ConsoleOut << "Enter your email: ";
        ConsoleOut.flush();
        adminEmail = ConsoleIn.readLine();
        if (QRegularExpression(".+@.+\\..+").match(adminEmail).hasMatch() == false) {
            ConsoleOut << "An invalid email was entered. Please try again." << endl;
            continue;
        }
        break;
    }
    while (true) {
        ConsoleOut << "Choose a Password: ";
        ConsoleOut.flush();
        QString password = Common::SecurePassInput();
        if (password.isEmpty()) {
            ConsoleOut << "Your password cannot be empty." << endl;
            continue;
        }
        ConsoleOut << "Re-enter your password: ";
        ConsoleOut.flush();
        QString password2 = Common::SecurePassInput();
        if (password != password2) {
            ConsoleOut << "The two entered passwords are not identical." << endl;
            continue;
        }
        password2.clear();
        adminEncPass = Common::GetPasswordHash(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha512).toHex().toLower());
        password.clear();

        break;
    }
    ConsoleOut << endl;

    ConsoleOut << "The next step is to decide where all messages from the server should be logged." << endl;
    ConsoleOut << "More granular control of what messages go where will be available using the web control panel." << endl << endl;
    bool logToStdOut = true;
    while (true) {
        ConsoleOut << "Would you like log messages to appear in a file or in the output from the server? (F)ile/(O)utput [o] ";
        ConsoleOut.flush();
        QString logPrompt = ConsoleIn.readLine();
        if (logPrompt.isEmpty() || logPrompt.startsWith('o', Qt::CaseInsensitive)) {
            logToStdOut = true;
            break;
        } else if (logPrompt.startsWith('f', Qt::CaseInsensitive)) {
            logToStdOut = false;
            break;
        } else {
            ConsoleOut << "Please enter either O to log messages to standard output or F to log to a file." << endl;
        }
    }

    QString logFilePath;
    if (!logToStdOut) {
        while (true) {
            ConsoleOut << "Please enter the path to the file where errors will be logged: ";
            ConsoleOut.flush();
            logFilePath = ConsoleIn.readLine();
            if (logFilePath.isEmpty()) {
                ConsoleOut << "You must enter a path to continue.";
                continue;
            }
            QFileInfo logFileInfo(logFilePath);
            if (logFileInfo.exists() && logFileInfo.isDir()) {
                ConsoleOut << "You have chosen a path to a directory, while a path to a file is required." << endl;
                continue;
            }
            if (logFileInfo.exists() == false && logFileInfo.dir().exists() == false) {
                if (logFileInfo.dir().mkpath(".") == false) {
                    ConsoleOut << "Can't create the parent folder(s) in the requested path. Do you have enough privileges?" << endl;
                    continue;
                }
            }
            break;
        }
    }
    ConsoleOut << endl;

    QString webpanelPath = _CONFIG_DIR "/webpanel";
    QFileInfo webpanelInfo(webpanelPath);
    if (webpanelInfo.exists() == false || webpanelInfo.isDir() == false || QDirIterator(webpanelPath, QStringList() << "*.php" << "*.html", QDir::Files, QDirIterator::Subdirectories).hasNext() == false) {
        ConsoleOut << "The web panel files don't seem to be at their default location. (Did you run `make install'?)" << endl;
        while (true) {
            ConsoleOut << "Please enter the path where the web panel files are located: ";
            ConsoleOut.flush();
            webpanelPath = ConsoleIn.readLine();
            if (webpanelPath.isEmpty()) {
                ConsoleOut << "You must enter a path to continue.";
                continue;
            }
            webpanelInfo.setFile(webpanelPath);
            if (webpanelInfo.exists() == false) {
                ConsoleOut << "You have entered a path to a non-existing directory. Please try again." << endl;
                continue;
            }
            if (webpanelInfo.isDir() == false) {
                ConsoleOut << "You have entered a path to a file instead of a directory. Please try again." << endl;
                continue;
            }
            if (QDir(webpanelPath).isEmpty()) {
                ConsoleOut << "Note that you have entered a path to an empty directory." << endl;
            }
            break;
        }
    }
    ConsoleOut << endl;

    ConsoleOut << "You are now at the end of the initialization process." << endl;
    ConsoleOut << "Please double-check your selections below." << endl;
    ConsoleOut << endl;
    ConsoleOut << "Server port:          " << port << endl;
    ConsoleOut << "SSL certificate path: " << certPath << endl;
    ConsoleOut << "Private key path:     " << keyPath << endl;
    ConsoleOut << "Admin name:           " << adminName << endl;
    ConsoleOut << "Admin email:          " << adminEmail << endl;
    if (logToStdOut)
    ConsoleOut << "Log Output:           Standard Output" << endl;
    else
    ConsoleOut << "Log Output:           " << logFilePath << endl;
    ConsoleOut << "Web panel files:      " << webpanelPath << endl;
    ConsoleOut << endl;
    ConsoleOut << "Are you ready to continue? (Y/N) ";
    ConsoleOut.flush();
    if (ConsoleIn.readLine().startsWith('y', Qt::CaseInsensitive) == false)
        exit(1);

    QSqlDatabase configDB = QSqlDatabase::addDatabase("QSQLITE");
    configDB.setDatabaseName(_CONFIG_DIR "/config.db");
    if (configDB.open() == false) {
        ConsoleOut << "Error! Failed to open database: " << configDB.lastError().text() << endl;
        exit(1);
    }
    QSqlQuery initQuery(configDB);
    QFile initScript(":/defaults/config.sql");
    if (initScript.open(QIODevice::ReadOnly)) {
        // The SQLite driver executes only the first query in the QSqlQuery.
        // If the script contains more queries, it needs to be split into multiple parts.
        QStringList scriptQueries = QTextStream(&initScript).readAll().split(';');

        foreach (QString queryTxt, scriptQueries) {
            if (queryTxt.trimmed().isEmpty()) {
                continue;
            }
            if (!initQuery.exec(queryTxt)) {
                ConsoleOut << "Error! Failed to initialize the database: " << initQuery.lastError().text() << endl;
                exit(1);
            }
            initQuery.finish();
        }
    }

    QSqlQuery addLog(configDB);
    addLog.prepare("INSERT OR IGNORE INTO `Logs`(Log_Name, Log_Path, Updt_User) VALUES (?, ?, ?);");
    addLog.addBindValue(tr("Default"));
    QUrl logOutput;
    if (logToStdOut)
        logOutput.setScheme("stdout");
    else {
        logOutput.setScheme("file");
        logOutput.setPath(logFilePath);
    }
    addLog.addBindValue(logOutput.toString());
    addLog.addBindValue(tr("Init"));
    if (addLog.exec() == false) {
        ConsoleOut << "Error! Failed to save the log output: " << addLog.lastError().text();
        exit(1);
    }
    if (addLog.numRowsAffected() == 0) {
        ConsoleOut << "Notice: Did not add new log output, since one to this location already exists." << endl;
    }
    foreach (QString cat, Log::Categories.keys()) {
        QSqlQuery addLogCat(configDB);
        addLogCat.prepare(R"(
            INSERT OR REPLACE INTO `Logs_Log_Categories`(Log_Key, Log_Category, Log_Levels)
            SELECT Log_Key, ? as Log_Category, ? as Log_Levels
            FROM `Logs`
            WHERE Log_Path = ?;)");
        addLogCat.addBindValue(cat);
        addLogCat.addBindValue(tr("error,warn,info"));
        addLogCat.addBindValue(logOutput.toString());
        if (addLogCat.exec() == false) {
            ConsoleOut << "Failed to set log output categories: " << addLogCat.lastError().text() << endl;
        }
    }

    ServerSettings::set("ListenPort", QString::number(port), "Init");
    ServerSettings::set("CertificatePath", certPath, "Init");
    ServerSettings::set("PrivateKeySource", "file", "Init");
    ServerSettings::set("PrivateKeyFilePath", keyPath, "Init");
    ServerSettings::set("WebPanelPath", webpanelPath, "Init");

    QSqlQuery addUser(configDB);
    addUser.prepare(R"(
        INSERT OR REPLACE INTO `Users`(FullName, Email, Password_Hash)
        VALUES (?, ?, ?);)");
    addUser.addBindValue(adminName);
    addUser.addBindValue(adminEmail);
    addUser.addBindValue(adminEncPass);
    if (addUser.exec() == false) {
        ConsoleOut << "Failed to add a new user: " << addUser.lastError().text() << endl;
    }
    QSqlQuery addRole(configDB);
    addRole.prepare(R"(
        INSERT OR REPLACE INTO `User_Roles`(User_Key, Role_Key)
        SELECT (SELECT seq FROM sqlite_sequence WHERE name = 'Users'), Role_Key
        FROM `Server_Roles`
        WHERE Role_Name = 'Administrator';)");
    if (addRole.exec() == false) {
        ConsoleOut << "Failed to add role user: " << addUser.lastError().text() << endl;
    }

    ConsoleOut << endl << "Your SSNFS server is now ready for use.";
}
