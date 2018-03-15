#ifndef LOG_H
#define LOG_H

#include <QString>
#include <QDebug>
#include <QHash>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlResult>
#include <QUrl>
#include <QDateTime>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <stdexcept>

QSqlDatabase getConfDB();

class LogOutput;

class LogCategory {
public:
    QString name;
    QString description;
    QVector<LogOutput> errorOutputs;
    QVector<QString> last10Errors;
    QVector<LogOutput> warningOutputs;
    QVector<LogOutput> infoOutputs;
    LogCategory() {}
    LogCategory(QString nme, QString desc) {
        name = nme;
        description = desc;
    }
    LogCategory(const LogCategory &copy) : LogCategory(copy.name, copy.description) {
        errorOutputs = copy.errorOutputs;
        warningOutputs = copy.warningOutputs;
        infoOutputs = copy.infoOutputs;
    }
};

class LogOutput {
public:
    enum OutputTypes {
        OutputFile,
        OutputSyslog,
        OutputStdOut,
        OutputStdErr
    };

    QString name;
    OutputTypes type;
    QUrl path;
    QDateTime updated;
    QString updatedBy;
    std::shared_ptr<spdlog::logger> logger;

    LogOutput() {}
    LogOutput(QString nme, QUrl output, QDateTime updateTmStmp, QString updatedByUser) {
        name = nme;
        if (output.scheme() == "file") {
            type = OutputFile;
            logger = spdlog::basic_logger_mt(name.toStdString(), path.path().toStdString());
        } else if (output.scheme() == "syslog") {
            type = OutputSyslog;
            logger = spdlog::syslog_logger(name.toStdString(), path.path().toStdString());
        } else if (output.scheme() == "stdout") {
            type = OutputStdOut;
            logger = spdlog::stdout_logger_mt(name.toStdString());
        } else if (output.scheme() == "stderr") {
            type = OutputStdErr;
            logger = spdlog::stderr_logger_mt(name.toStdString());
        } else {
            throw std::invalid_argument("output");
        }
        updated = updateTmStmp;
        updatedBy = updatedByUser;
    }
};

class Log
{
public:

    static QHash<QString, LogCategory> Categories;
    static QVector<LogOutput> Outputs;

    static bool isInit;

    static inline void init() {
        if (isInit) {
            qWarning("Log init is called more than once.");
            return;
        }

        // Categories must be added here.
        Categories.insert("Connection", LogCategory("Connection", "Messages related to client connections."));
        Categories.insert("Authentication", LogCategory("Authentication", "Messages generated during client authentication."));
        Categories.insert("File System", LogCategory("File System", "Various technical messages generated while processing requests."));
        Categories.insert("Server", LogCategory("Server", "Messages related to the server status including configuration changes."));

        QSqlDatabase configDB = getConfDB();
        if (!configDB.isValid()) {
            exit(1);
        }
        // Do a very basic check if the log tables are there and have the right columns.
        QSqlQuery checkTables(
                    R"(SELECT COUNT(*)
                       FROM (
                            SELECT 1 FROM sqlite_master WHERE type='table' AND name='Logs' AND `sql` LIKE '%Log_Key%Log_Name%Log_Path%Updt_TmStmp%Updt_User%'
                            UNION ALL
                            SELECT 1 FROM sqlite_master WHERE type='table' AND name='Logs_Log_Categories' AND `sql` LIKE '%Log_Key%Log_Category%Log_Levels%'
                            );
                    )");
        if (!checkTables.exec() || !checkTables.next() || checkTables.value(0).toInt() != 2) {
            qCritical() << "Error validating configuration database schema: The Logs and/or Logs_Log_Categories tables do not have the required columns.";
            exit(1);
        }
        checkTables.clear();
        
        QSqlQuery logsQuery(
                    R"(SELECT `Log_Key`, `Log_Name`, `Log_Path`, `Updt_TmStmp`, `Updt_User`
                       FROM `Logs`;
                    )");
        if (!logsQuery.exec()) {
            qCritical() << "Error while retrieving Log configuration from DB:" << logsQuery.lastError().text();
            exit(1);
        }
        while (logsQuery.next()) {
            try {
                Outputs.append(LogOutput(logsQuery.value(1).toString(), QUrl(logsQuery.value(2).toString()), logsQuery.value(3).toDateTime(), logsQuery.value(4).toString()));

                QSqlQuery getCategories(
                            R"(SELECT `Log_Category`, `Log_Levels`
                               FROM `Logs_Log_Categories`
                               WHERE `Log_Key` = ?;
                            )");
                getCategories.addBindValue(logsQuery.value(0));
                if (!getCategories.exec()) {
                    qCritical() << "Error while retrieving Log Categories from DB:" << logsQuery.lastError().text();
                    exit(1);
                }
                while (getCategories.next()) {
                    if (getCategories.value(1).toString().contains("error"))
                        Categories[getCategories.value(0).toString()].errorOutputs.append(Outputs.last());
                    if (getCategories.value(1).toString().contains("warn"))
                        Categories[getCategories.value(0).toString()].warningOutputs.append(Outputs.last());
                    if (getCategories.value(1).toString().contains("info"))
                        Categories[getCategories.value(0).toString()].infoOutputs.append(Outputs.last());
                }
                Outputs.last().logger->info("Opened {0} for logging.", QUrl(logsQuery.value(2).toString()).scheme().toUtf8().data());
            } catch (std::invalid_argument) {
                qWarning() << "Warning: Unable to identify log backend for uri:" << logsQuery.value(1).toString();
            }
        }

        isInit = true;
    }

    template <typename... Args> static inline void error(const LogCategory& category, const char* fmt, const Args&... args) {
        if (!isInit) {
            qWarning() << "Warning: Log called before init. Running init now.";
            init();
        }

        for (int i = 0; i < category.errorOutputs.length(); i++) {
            category.errorOutputs[i].logger->error(fmt, args...);
        }
    }
    template <typename... Args> static inline void warn(const LogCategory& category, const char* fmt, const Args&... args) {
        if (!isInit) {
            qWarning() << "Warning: Log called before init. Running init now.";
            init();
        }

        for (int i = 0; i < category.warningOutputs.length(); i++) {
            category.warningOutputs[i].logger->warn(fmt, args...);
        }
    }
    template <typename... Args> static inline void info(const LogCategory& category, const char* fmt, const Args&... args) {
        if (!isInit) {
            qWarning() << "Warning: Log called before init. Running init now.";
            init();
        }

        for (int i = 0; i < category.infoOutputs.length(); i++) {
            category.infoOutputs[i].logger->info(fmt, args...);
        }
    }
};


#endif // LOG_H
