/*
 * SSNFS Client v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2018 Maxwell Dreytser
 */

#include "ssnfsworker.h"
#include <log.h>
#include <serversettings.h>
#include <common.h>

#include <ph7.h>
#include <fastpbkdf2.h>
#include <QMimeDatabase>
#include <QFileInfo>

static int PH7Consumer(const void *pOutput, unsigned int nOutputLen, void *pUserData) {
    ((SSNFSWorker*)pUserData)->httpResponse.append((const char*)pOutput, nOutputLen);
    return PH7_OK;
}
static void PH7ErrorLogger(const char *zMessage, int iMsgType, const char *zDest, const char *zHeader) {
    Log::warn(Log::Categories["Web Server"], "Error {0}: {1} - Dest {2}; Header {3}", iMsgType, zMessage, zDest, zHeader);
}
static int PH7SetHeader(ph7_context *pCtx,int argc,ph7_value **argv) {
    SSNFSWorker *worker = (SSNFSWorker*)ph7_context_user_data(pCtx);
    if (argc >= 1 && ph7_value_is_string(argv[0])) {
        int headerLen;
        const char *headerStr = ph7_value_to_string(argv[0], &headerLen);
        QVector<QString> headerParts;
        QString header(QByteArray(headerStr, headerLen));
        int HeaderNameLen = header.indexOf(": ");
        if (HeaderNameLen == -1) {
            ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "The specified header does not contain \": \". Header key/value pairs must be separated by a \": \".");
            ph7_result_bool(pCtx, 0);
            return PH7_OK;
        }
        bool replace = true;
        if (argc >= 2 && ph7_value_is_bool(argv[1]) && ph7_value_to_bool(argv[1]) == 0)
            replace = false;

        if (replace) {
            worker->responseHeaders.insert(header.mid(0, HeaderNameLen), header.mid(HeaderNameLen + 2));
        } else {
            worker->responseHeaders.insertMulti(header.mid(0, HeaderNameLen), header.mid(HeaderNameLen + 2));
        }
    }

    ph7_result_bool(pCtx, 1);
    return PH7_OK;
}
static int PH7ResponseCode(ph7_context *pCtx,int argc,ph7_value **argv) {
    SSNFSWorker *worker = (SSNFSWorker*)ph7_context_user_data(pCtx);
    ph7_result_int(pCtx, worker->httpResultCode);
    if (argc >= 1 && ph7_value_is_int(argv[0])) {
        int newCode = ph7_value_to_int(argv[0]);
        if (worker->knownResultCodes.keys().contains(newCode)) {
            worker->httpResultCode = newCode;
        } else {
            ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "Invalid or unsupported HTTP return code specified.");
        }
    }
    return PH7_OK;
}
static int PH7CheckAuthCookie(ph7_context *pCtx,int argc,ph7_value **argv) {
    SSNFSWorker *worker = (SSNFSWorker*)ph7_context_user_data(pCtx);

    if (argc == 1 && ph7_value_is_string(argv[0])) {
        QString cookie(ph7_value_to_string(argv[0], 0));
        if (cookie.isNull() || cookie.isEmpty()) {
            ph7_result_bool(pCtx, false);

            return PH7_OK;
        }

        QSqlQuery getUserKey(worker->configDB);
        getUserKey.prepare(R"(
            SELECT t.`User_Key`, rp.`Perm_ShortName`
            FROM `Web_Tokens` t
            LEFT JOIN `User_Roles` ur
            ON `Token` = ? AND ((julianday('now') - julianday(LastAccess_TmStmp)) * 24 * 60) < 30 AND t.`User_Key` = ur.`User_Key`
            LEFT JOIN `Server_Role_Perms` rp
            ON ur.`Role_Key` = rp.`Role_Key`;)");
        getUserKey.addBindValue(cookie);
        if (getUserKey.exec()) {
            if (getUserKey.next()) {
                worker->userKey = getUserKey.value(0).toLongLong();

                ph7_result_bool(pCtx, true);

                do {
                    if (getUserKey.isNull(1) == false)
                        worker->userPerms.append(getUserKey.value(1).toString());
                } while (getUserKey.next());

                QSqlQuery updateTokenTmStmp(worker->configDB);
                updateTokenTmStmp.prepare(R"(
                    UPDATE `Web_Tokens`
                    SET `LastAccess_TmStmp` = CURRENT_TIMESTAMP
                    WHERE `Token` = ?)");
                updateTokenTmStmp.addBindValue(cookie);
                updateTokenTmStmp.exec();
            } else {
                ph7_result_bool(pCtx, false);
            }
        } else {
            Log::error(Log::Categories["Web Server"], "Error while checking auth cookie: {0}", ToChr(getUserKey.lastError().text()));
            ph7_context_throw_error(pCtx, PH7_CTX_ERR, "Internal error while verifying the auth cookie.");
        }
    } else {
        ph7_context_throw_error(pCtx, PH7_CTX_ERR, "An invalid number or type of argument(s) was specified.");
    }

    return PH7_OK;
}

static int PH7MakeAuthCookie(ph7_context *pCtx,int argc,ph7_value **argv) {
    SSNFSWorker *worker = (SSNFSWorker*)ph7_context_user_data(pCtx);

    if (argc == 2 && ph7_value_is_string(argv[0]) && ph7_value_is_string(argv[1])) {
        int strEmailLen;
        const char *strEmail = ph7_value_to_string(argv[0], &strEmailLen);
        int strClientPasswdLen;
        const char *strClientPasswd = ph7_value_to_string(argv[1], &strClientPasswdLen);

        qint64 userKey = SSNFSWorker::checkEmailPass(worker->configDB, QString(strEmail), QString(strClientPasswd));
        if (userKey > -1) {
            while (true) {
                QString newCookie = QUuid::createUuid().toString();
                QSqlQuery addCookie(worker->configDB);
                addCookie.prepare("INSERT OR IGNORE INTO `Web_Tokens`(`User_Key`,`Token`) VALUES (?,?);");
                addCookie.addBindValue(userKey);
                addCookie.addBindValue(newCookie);
                if (addCookie.exec()) {
                    if (addCookie.numRowsAffected() != 0) {
                        QByteArray newCookieBytes = newCookie.toUtf8();
                        ph7_result_string(pCtx, newCookieBytes.data(), newCookieBytes.length());
                        break;
                    }
                } else {
                    Log::error(Log::Categories["Web Server"], "Error while adding a new auth cookie: {0}", ToChr(addCookie.lastError().text()));
                    ph7_context_throw_error(pCtx, PH7_CTX_ERR, "Internal error while generating an auth cookie.");
                    break;
                }
            }
        } else {
            ph7_result_string(pCtx, "", 0);
        }
    } else {
        ph7_context_throw_error(pCtx, PH7_CTX_ERR, "An invalid number or type of argument(s) was specified.");
    }

    return PH7_OK;
}

static int PH7DelAuthCookie(ph7_context *pCtx,int argc,ph7_value **argv) {
    SSNFSWorker *worker = (SSNFSWorker*)ph7_context_user_data(pCtx);

    if (argc == 1 && ph7_value_is_string(argv[0])) {
        QString cookie(ph7_value_to_string(argv[0], 0));
        if (cookie.isNull() || cookie.isEmpty()) {
            ph7_result_bool(pCtx, false);

            return PH7_OK;
        }

        QSqlQuery delToken(worker->configDB);
        delToken.prepare("DELETE FROM `Web_Tokens` WHERE `Token` = ?;");
        delToken.addBindValue(cookie);
        if (delToken.exec() == false) {
            Log::error(Log::Categories["Web Server"], "Error removing token during logout: {0}", ToChr(delToken.lastError().text()));
            ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "An internal error occured while deleting the requested token.");
        }
    } else {
        ph7_context_throw_error(pCtx, PH7_CTX_ERR, "An invalid number or type of argument(s) was specified.");
    }

    return PH7_OK;
}

static int PH7GetConnected(ph7_context *pCtx,int argc,ph7_value **argv) {
    (void) argc;
    (void) argv;

    SSNFSWorker *worker = (SSNFSWorker*)ph7_context_user_data(pCtx);

    ph7_value *clients = ph7_context_new_array(pCtx);
    foreach (const QObject *currChild, worker->parent()->children()) {
        if (currChild->objectName() == "Connected") {
            const SSNFSWorker *currWorker = (const SSNFSWorker*)currChild;
            if (worker->userPerms.contains("connected") == false && currWorker->userKey != worker->userKey)
                continue;
            ph7_value *clientInfo = ph7_context_new_array(pCtx);
            ph7_value *userKeyVal = ph7_context_new_scalar(pCtx);
            ph7_value_int64(userKeyVal, worker->userKey);
            ph7_array_add_strkey_elem(clientInfo, "userKey", userKeyVal);
            ph7_value *userNameVal = ph7_context_new_scalar(pCtx);
            ph7_value_string(userNameVal, ToChr(currWorker->userName), -1);
            ph7_array_add_strkey_elem(clientInfo, "userName", userNameVal);
            ph7_value *clientKeyVal = ph7_context_new_scalar(pCtx);
            ph7_value_int64(clientKeyVal, worker->clientKey);
            ph7_array_add_strkey_elem(clientInfo, "clientKey", clientKeyVal);
            ph7_value *clientNameVal = ph7_context_new_scalar(pCtx);
            ph7_value_string(clientNameVal, ToChr(currWorker->clientName), -1);
            ph7_array_add_strkey_elem(clientInfo, "clientName", clientNameVal);
            ph7_value *shareKeyVal = ph7_context_new_scalar(pCtx);
            ph7_value_int64(shareKeyVal, worker->shareKey);
            ph7_array_add_strkey_elem(clientInfo, "shareKey", shareKeyVal);
            ph7_value *shareNameVal = ph7_context_new_scalar(pCtx);
            ph7_value_string(shareNameVal, ToChr(currWorker->shareName), -1);
            ph7_array_add_strkey_elem(clientInfo, "shareName", shareNameVal);
            ph7_array_add_elem(clients, NULL, clientInfo);
        }
    }
    ph7_result_value(pCtx, clients);

    return PH7_OK;
}

static int PH7GetPending(ph7_context *pCtx,int argc,ph7_value **argv) {
    (void) argc;
    (void) argv;

    SSNFSWorker *worker = (SSNFSWorker*)ph7_context_user_data(pCtx);

    ph7_value *pending = ph7_context_new_array(pCtx);
    QSqlQuery getPending(worker->configDB);
    if (worker->userPerms.contains("computers")) {
        getPending.prepare(R"(
            SELECT `Pending_Client_Key`,
                   u.`FullName`,
                   `Client_Name`,
                   CAST(strftime('%s',`Submit_TmStmp`) AS INTEGER),
                   `Submit_Host`
            FROM `Pending_Clients` c
            JOIN `Users` u
            ON c.`User_Key` = u.`User_Key`;)");
    } else {
        getPending.prepare(R"(
            SELECT `Pending_Client_Key`,
                   u.`FullName`,
                   `Client_Name`,
                   CAST(strftime('%s',`Submit_TmStmp`) AS INTEGER),
                   `Submit_Host`
            FROM `Pending_Clients` c
            JOIN `Users` u
            ON c.`User_Key` = ? AND c.`User_Key` = u.`User_Key`;)");
        getPending.addBindValue(worker->userKey);
    }
    if (getPending.exec()) {
        while (getPending.next()) {
            ph7_value *clientInfo = ph7_context_new_array(pCtx);
            ph7_value *pendingClientKeyVal = ph7_context_new_scalar(pCtx);
            ph7_value_int64(pendingClientKeyVal, getPending.value(0).toLongLong());
            ph7_array_add_strkey_elem(clientInfo, "pendingClientKey", pendingClientKeyVal);
            ph7_value *userNameVal = ph7_context_new_scalar(pCtx);
            ph7_value_string(userNameVal, ToChr(getPending.value(1).toString()), -1);
            ph7_array_add_strkey_elem(clientInfo, "userName", userNameVal);
            ph7_value *clientNameVal = ph7_context_new_scalar(pCtx);
            ph7_value_string(clientNameVal, ToChr(getPending.value(2).toString()), -1);
            ph7_array_add_strkey_elem(clientInfo, "clientName", clientNameVal);
            ph7_value *submitTmStmpVal = ph7_context_new_scalar(pCtx);
            ph7_value_int64(submitTmStmpVal, getPending.value(3).toLongLong());
            ph7_array_add_strkey_elem(clientInfo, "submitTmStmp", submitTmStmpVal);
            ph7_value *submitHostVal = ph7_context_new_scalar(pCtx);
            ph7_value_string(submitHostVal, ToChr(getPending.value(4).toString()), -1);
            ph7_array_add_strkey_elem(clientInfo, "submitHost", submitHostVal);
            ph7_array_add_elem(pending, NULL, clientInfo);
        }
    } else {
        Log::error(Log::Categories["Web Server"], "Error while getting pending computers: {0}", ToChr(getPending.lastError().text()));
        ph7_context_throw_error(pCtx, PH7_CTX_ERR, "Internal error while getting pending computers.");
    }

    ph7_result_value(pCtx, pending);

    return PH7_OK;
}

static int PH7ApprovePending(ph7_context *pCtx,int argc,ph7_value **argv) {
    SSNFSWorker *worker = (SSNFSWorker*)ph7_context_user_data(pCtx);

    if (argc < 1 || argc > 2 || ph7_value_is_numeric(argv[0]) == false || (argc == 2 && ph7_value_is_string(argv[1]) == false)) {
        ph7_context_throw_error(pCtx, PH7_CTX_ERR, "An invalid number of argument(s) was specified.");
    } else {
        qint64 pendingClientKey = ph7_value_to_int64(argv[0]);
        QSqlQuery approvePend(worker->configDB);
        if (argc == 1) {
            if (worker->userPerms.contains("computers")) {
                approvePend.prepare(R"(
                    INSERT OR REPLACE INTO Clients
                      (`User_Key`
                      ,`Client_Name`
                      ,`Client_Cert`
                      ,`Client_Info`
                      ,`Updt_User`)
                    SELECT c.`User_Key`
                      ,`Client_Name`
                      ,`Client_Cert`
                      ,`Client_Info`
                      ,u.`FullName`
                    FROM `Pending_Clients` c
                    JOIN `Users` u
                    ON c.`Pending_Client_Key` = ? AND u.`User_Key` = ?)");
            } else {
                approvePend.prepare(R"(
                    INSERT OR REPLACE INTO Clients
                      (`User_Key`
                      ,`Client_Name`
                      ,`Client_Cert`
                      ,`Client_Info`
                      ,`Updt_User`)
                    SELECT c.`User_Key`
                      ,`Client_Name`
                      ,`Client_Cert`
                      ,`Client_Info`
                      ,u.`FullName`
                    FROM `Pending_Clients` c
                    JOIN `Users` u
                    ON c.`Pending_Client_Key` = ? AND c.`User_Key` = ? AND u.`User_Key` = c.`User_Key`)");
            }
            approvePend.addBindValue(pendingClientKey);
            approvePend.addBindValue(worker->userKey);
        } else {
            if (worker->userPerms.contains("computers")) {
                approvePend.prepare(R"(
                    INSERT OR REPLACE INTO Clients
                      (`User_Key`
                      ,`Client_Name`
                      ,`Client_Cert`
                      ,`Client_Info`
                      ,`Updt_User`)
                    SELECT c.`User_Key`
                      ,? as Client_Name
                      ,`Client_Cert`
                      ,`Client_Info`
                      ,u.`FullName`
                    FROM `Pending_Clients` c
                    JOIN `Users` u
                    ON c.`Pending_Client_Key` = ? AND u.`User_Key` = ?)");
            } else {
                approvePend.prepare(R"(
                    INSERT OR REPLACE INTO Clients
                      (`User_Key`
                      ,`Client_Name`
                      ,`Client_Cert`
                      ,`Client_Info`
                      ,`Updt_User`)
                    SELECT c.`User_Key`
                      ,? as Client_Name
                      ,`Client_Cert`
                      ,`Client_Info`
                      ,u.`FullName`
                    FROM `Pending_Clients` c
                    JOIN `Users` u
                    ON c.`Pending_Client_Key` = ? AND c.`User_Key` = ? AND u.`User_Key` = c.`User_Key`)");
            }
            approvePend.addBindValue(QString(ph7_value_to_string(argv[1], NULL)));
            approvePend.addBindValue(pendingClientKey);
            approvePend.addBindValue(worker->userKey);
        }
        if (approvePend.exec()) {
            if (approvePend.numRowsAffected() == 0) {
                ph7_result_string(pCtx, "The computer was not approved. The computer may already be approved or you may not have permissions to approve such computers.", -1);
            } else {
                QSqlQuery delApproved(worker->configDB);
                delApproved.prepare("DELETE FROM `Pending_Clients` WHERE `Pending_Client_Key` = ?");
                delApproved.addBindValue(pendingClientKey);
                if (delApproved.exec()) {
                    ph7_result_string(pCtx, "OK", -1);
                } else {
                    Log::error(Log::Categories["Web Server"], "Error while deleting an approved pending computer: {0}", ToChr(approvePend.lastError().text()));
                    ph7_context_throw_error(pCtx, PH7_CTX_ERR, "Internal error while deleting an approved pending computer.");
                }
            }
        } else {
            Log::error(Log::Categories["Web Server"], "Error while approving a pending computer: {0}", ToChr(approvePend.lastError().text()));
            ph7_context_throw_error(pCtx, PH7_CTX_ERR, "Internal error while approving a pending computer.");
        }
    }

    return PH7_OK;
}

static int PH7CheckPerm(ph7_context *pCtx,int argc,ph7_value **argv) {
    SSNFSWorker *worker = (SSNFSWorker*)ph7_context_user_data(pCtx);

    if (argc == 1 && ph7_value_is_string(argv[0])) {
        ph7_result_bool(pCtx, worker->userPerms.contains(QString(ph7_value_to_string(argv[0], NULL))));
    } else {
        ph7_context_throw_error(pCtx, PH7_CTX_ERR, "An invalid number of argument(s) was specified.");
    }

    return PH7_OK;
}

static int PH7GetShares(ph7_context *pCtx,int argc,ph7_value **argv) {
    SSNFSWorker *worker = (SSNFSWorker*)ph7_context_user_data(pCtx);

    QSqlQuery getShares(worker->configDB);
    if (argc == 1) {
        if (ph7_value_is_int(argv[0])) {
            getShares.prepare(R"(
                SELECT s.`Share_Key`,
                       `Share_Name`,
                       `Local_Path`,
                       CAST(strftime('%s',s.`Updt_TmStmp`) AS INTEGER),
                       s.`Updt_User`,
                       us.`User_Key`,
                       us.`Default_Perms`,
                       u.`FullName`,
                       u.`Email`
                FROM `Shares` s
                LEFT JOIN `User_Shares` us
                ON s.`Share_Key` = us.`Share_Key`
                LEFT JOIN `Users` u
                ON us.`User_Key` = u.`User_Key`
                WHERE s.`Share_Key` = ?;)");
            getShares.addBindValue(ph7_value_to_int64(argv[0]));
        } else if (ph7_value_is_string(argv[0])) {
            getShares.prepare(R"(
                SELECT s.`Share_Key`,
                       `Share_Name`,
                       `Local_Path`,
                       CAST(strftime('%s',s.`Updt_TmStmp`) AS INTEGER),
                       s.`Updt_User`,
                       us.`User_Key`,
                       us.`Default_Perms`,
                       u.`FullName`,
                       u.`Email`
                FROM `Shares` s
                LEFT JOIN `User_Shares` us
                ON s.`Share_Key` = us.`Share_Key`
                LEFT JOIN `Users` u
                ON us.`User_Key` = u.`User_Key`
                WHERE s.`Share_Name` = ?;)");
            getShares.addBindValue(QString(ph7_value_to_string(argv[0], NULL)));
        } else {
            ph7_context_throw_error(pCtx, PH7_CTX_ERR, "If you specify an argument to this function, it must be either an integer or a string.");
            return PH7_OK;
        }
    } else if (argc == 0) {
        getShares.prepare(R"(
            SELECT s.`Share_Key`,
                   `Share_Name`,
                   `Local_Path`,
                   CAST(strftime('%s',s.`Updt_TmStmp`) AS INTEGER),
                   s.`Updt_User`,
                   us.`User_Key`,
                   us.`Default_Perms`,
                   u.`FullName`,
                   u.`Email`
            FROM `Shares` s
            LEFT JOIN `User_Shares` us
            ON s.`Share_Key` = us.`Share_Key`
            LEFT JOIN `Users` u
            ON us.`User_Key` = u.`User_Key`;)");
    } else {
        ph7_context_throw_error(pCtx, PH7_CTX_ERR, "You have specified an invalid number of arguments to this function.");
        return PH7_OK;
    }

    if (getShares.exec()) {
        ph7_value *returnVal = ph7_context_new_array(pCtx);
        qint64 lastShareKey = -1;
        ph7_value *currShare = nullptr;
        ph7_value *currShareUsers = nullptr;
        while (getShares.next()) {
            qint64 currShareKey = getShares.value(0).toLongLong();
            if (lastShareKey != currShareKey) {
                if (currShareUsers != nullptr)
                    ph7_array_add_strkey_elem(currShare, "users", currShareUsers);
                if (currShare != nullptr)
                    ph7_array_add_elem(returnVal, NULL, currShare);

                currShare = ph7_context_new_array(pCtx);

                ph7_value *shareKeyVal = ph7_context_new_scalar(pCtx);
                ph7_value_int64(shareKeyVal, currShareKey);
                ph7_array_add_strkey_elem(currShare, "shareKey", shareKeyVal);
                ph7_value *shareNameVal = ph7_context_new_scalar(pCtx);
                ph7_value_string(shareNameVal, ToChr(getShares.value(1).toString()), -1);
                ph7_array_add_strkey_elem(currShare, "shareName", shareNameVal);
                ph7_value *localPathVal = ph7_context_new_scalar(pCtx);
                ph7_value_string(localPathVal, ToChr(getShares.value(2).toString()), -1);
                ph7_array_add_strkey_elem(currShare, "localPath", localPathVal);
                ph7_value *updtTmStmpVal = ph7_context_new_scalar(pCtx);
                ph7_value_int64(updtTmStmpVal, getShares.value(3).toLongLong());
                ph7_array_add_strkey_elem(currShare, "updtTmStmp", updtTmStmpVal);
                ph7_value *updtUserVal = ph7_context_new_scalar(pCtx);
                ph7_value_string(updtUserVal, ToChr(getShares.value(4).toString()), -1);
                ph7_array_add_strkey_elem(currShare, "updtUser", updtUserVal);

                currShareUsers = ph7_context_new_array(pCtx);

                lastShareKey = currShareKey;
            }

            if (getShares.isNull(5) == false) {
                ph7_value *currShareUser = ph7_context_new_array(pCtx);
                ph7_value *userKeyVal = ph7_context_new_scalar(pCtx);
                ph7_value_int64(userKeyVal, getShares.value(5).toLongLong());
                ph7_array_add_strkey_elem(currShareUser, "userKey", userKeyVal);
                ph7_value *defaultPermsVal = ph7_context_new_scalar(pCtx);
                ph7_value_string(defaultPermsVal, ToChr(getShares.value(6).toString()), -1);
                ph7_array_add_strkey_elem(currShareUser, "defaultPerms", defaultPermsVal);
                ph7_value *userNameVal = ph7_context_new_scalar(pCtx);
                ph7_value_string(userNameVal, ToChr(getShares.value(7).toString()), -1);
                ph7_array_add_strkey_elem(currShareUser, "userName", userNameVal);
                ph7_value *emailVal = ph7_context_new_scalar(pCtx);
                ph7_value_string(emailVal, ToChr(getShares.value(8).toString()), -1);
                ph7_array_add_strkey_elem(currShareUser, "email", emailVal);

                ph7_array_add_elem(currShareUsers, NULL, currShareUser);
            }
        }
        if (currShareUsers != nullptr)
            ph7_array_add_strkey_elem(currShare, "users", currShareUsers);
        if (currShare != nullptr)
            ph7_array_add_elem(returnVal, NULL, currShare);
        ph7_result_value(pCtx, returnVal);
    } else {
        Log::error(Log::Categories["Web Server"], "Error while retrieving shares: {0}", ToChr(getShares.lastError().text()));
        ph7_context_throw_error(pCtx, PH7_CTX_ERR, "Internal error while retrieving shares.");
        qInfo() << getShares.executedQuery();
    }

    return PH7_OK;
}

struct shareuserswalk1data {
    qint64 shareKey;
    QString *errorMsg;
    SSNFSWorker *worker;
    QHash<qint64, QString> *shareUsers;
};
static int shareUsersWalk1(ph7_value *pKey,ph7_value *pValue,void *pUserData) {
    struct shareuserswalk1data *pData = (struct shareuserswalk1data*)pUserData;
    ph7_value *userKeyVal = ph7_array_fetch(pValue, "userKey", -1);
    ph7_value *userNameVal = ph7_array_fetch(pValue, "userName", -1);
    ph7_value *emailVal = ph7_array_fetch(pValue, "email", -1);
    ph7_value *permsVal = ph7_array_fetch(pValue, "defaultPerms", -1);
    ph7_value *isDeleted = ph7_array_fetch(pValue, "deleted", -1);
    if (userKeyVal == NULL || ph7_value_is_int(userKeyVal) == false ||
            userNameVal == NULL || ph7_value_is_string(userNameVal) == false ||
            emailVal == NULL || ph7_value_is_string(emailVal) == false ||
            permsVal == NULL || ph7_value_is_string(permsVal) == false) {
        pData->errorMsg->append("Invalid share user.");
        return PH7_ABORT;
    }
    qint64 userKey = ph7_value_to_int64(userKeyVal);
    QString userName = ph7_value_to_string(userNameVal, NULL);
    QString email = ph7_value_to_string(emailVal, NULL);
    QString defaultPerms = ph7_value_to_string(permsVal, NULL);
    if (isDeleted != NULL && ph7_value_is_bool(isDeleted) && ph7_value_to_bool(isDeleted)) {
        QSqlQuery delUser(pData->worker->configDB);
        delUser.prepare(R"(
            DELETE FROM `User_Shares`
            WHERE `User_Share_Key` IN (
                SELECT `User_Share_Key`
                FROM `User_Shares` us
                JOIN `Users` u
                ON us.`Share_Key` = ? AND us.`User_Key` = ? AND us.`User_Key` = u.`User_Key` AND u.`FullName` = ? AND u.`Email` = ?);)");
        delUser.addBindValue(pData->shareKey);
        delUser.addBindValue(userKey);
        delUser.addBindValue(userName);
        delUser.addBindValue(email);
        if (delUser.exec() == false) {
            pData->errorMsg->append("Internal error while deleting the specified share user.");
            Log::error(Log::Categories["Web Server"], "Error while deleting the specified share user: {0}", ToChr(delUser.lastError().text()));
            return PH7_ABORT;
        }
        return PH7_OK;
    }
    QSqlQuery checkUser(pData->worker->configDB);
    checkUser.prepare(R"(
        SELECT 1
        FROM `Users`
        WHERE `User_Key` = ? AND `FullName` = ? AND `Email` = ?;)");
    checkUser.addBindValue(userKey);
    checkUser.addBindValue(userName);
    checkUser.addBindValue(email);
    if (checkUser.exec()) {
        if (checkUser.next() == false) {
            pData->errorMsg->append("A specified share user does not match any existing user.");
            return PH7_ABORT;
        }
    } else {
        pData->errorMsg->append("Internal error while verifying the provided share user.");
        Log::error(Log::Categories["Web Server"], "Error while verifying a provided share user: {0}", ToChr(checkUser.lastError().text()));
        return PH7_ABORT;
    }

    pData->shareUsers->insert(userKey, defaultPerms);

    return PH7_OK;
}
static int PH7SaveShare(ph7_context *pCtx,int argc,ph7_value **argv) {
    SSNFSWorker *worker = (SSNFSWorker*)ph7_context_user_data(pCtx);

    if (argc == 4 && ph7_value_is_int(argv[0]) && ph7_value_is_string(argv[1]) && ph7_value_is_string(argv[2]) && ph7_value_is_array(argv[3])) {
        if (worker->userPerms.contains("shares")) {
            qint64 shareKey = ph7_value_to_int64(argv[0]);
            QString shareName = ph7_value_to_string(argv[1], NULL);
            QString localPath = ph7_value_to_string(argv[2], NULL);

            struct shareuserswalk1data walkData;
            walkData.worker = worker;
            walkData.shareKey = shareKey;
            QHash<qint64, QString> shareUsers;
            walkData.shareUsers = &shareUsers;
            QString walkErrorMsg;
            walkData.errorMsg = &walkErrorMsg;
            if (ph7_array_walk(argv[3], &shareUsersWalk1, &walkData) != PH7_OK) {
                ph7_result_string(pCtx, ToChr(walkErrorMsg), -1);
                return PH7_OK;
            }

            QFileInfo pathInfo(localPath);
            if (shareKey < -1) {
                ph7_result_string(pCtx, "You have specified an invalid share key.", -1);
                return PH7_OK;
            } else if (pathInfo.exists() == false) {
                ph7_result_string(pCtx, "The path that you have specified does not exist.", -1);
                return PH7_OK;
            } else if (pathInfo.isDir() == false) {
                ph7_result_string(pCtx, "The path that you have specified does not point to a directory.", -1);
                return PH7_OK;
            } else if (shareKey == -1) {
                QSqlQuery addShare(worker->configDB);
                addShare.prepare(R"(
                    INSERT OR IGNORE INTO `Shares`(`Share_Name`, `Local_Path`, `Updt_User`)
                    SELECT ? AS Share_Name,
                           ? AS Local_Path,
                           FullName
                    FROM `Users`
                    WHERE `User_Key` = ?;)");
                addShare.addBindValue(shareName);
                addShare.addBindValue(localPath);
                addShare.addBindValue(worker->userKey);
                if (addShare.exec()) {
                    if (addShare.numRowsAffected() == 1) {
                        QSqlQuery getShareKey(worker->configDB);
                        getShareKey.prepare(R"(
                            SELECT CAST(`seq` as INTEGER)
                            FROM `sqlite_sequence`
                            WHERE `name` = 'Shares';)");
                        if (getShareKey.exec()) {
                            if (getShareKey.next()) {
                                shareKey = getShareKey.value(0).toLongLong();
                            } else {
                                Log::error(Log::Categories["Web Server"], "Error while retrieving the key of a newly added share: Could not find 'Shares' table in `sqlite_sequence`!", ToChr(addShare.lastError().text()));
                                ph7_context_throw_error(pCtx, PH7_CTX_ERR, "Internal error while adding new share.");
                                return PH7_OK;
                            }
                        } else {
                            Log::error(Log::Categories["Web Server"], "Error while retrieving the key of a newly added share: {0}", ToChr(addShare.lastError().text()));
                            ph7_context_throw_error(pCtx, PH7_CTX_ERR, "Internal error while adding new share.");
                            return PH7_OK;
                        }
                    } else {
                        ph7_result_string(pCtx, "A share with this name already exists.", -1);
                        return PH7_OK;
                    }
                } else {
                    Log::error(Log::Categories["Web Server"], "Error while adding new share: {0}", ToChr(addShare.lastError().text()));
                    ph7_context_throw_error(pCtx, PH7_CTX_ERR, "Internal error while adding new share.");
                    return PH7_OK;
                }
            } else {
                QSqlQuery updtShare(worker->configDB);
                updtShare.prepare(R"(
                    UPDATE `Shares`
                    SET `Share_Name` = ?,
                        `Local_Path` = ?,
                        `Updt_TmStmp` = CURRENT_TIMESTAMP,
                        `Updt_User` = (SELECT `FullName` FROM `Users` WHERE `User_Key` = ?)
                    WHERE `Share_Key` = ?;)");
                updtShare.addBindValue(shareName);
                updtShare.addBindValue(localPath);
                updtShare.addBindValue(worker->userKey);
                updtShare.addBindValue(shareKey);
                if (updtShare.exec()) {
                    if (updtShare.numRowsAffected() != 1) {
                        ph7_result_string(pCtx, "A share with the specified share key does not exist.", -1);
                    }
                } else {
                    Log::error(Log::Categories["Web Server"], "Error while updating the share: {0}", ToChr(updtShare.lastError().text()));
                    ph7_context_throw_error(pCtx, PH7_CTX_ERR, "Internal error while updating the share.");
                    return PH7_OK;
                }
            }

            for (QHash<qint64, QString>::iterator i = shareUsers.begin(); i != shareUsers.end(); ++i) {
                QSqlQuery addShareUser(worker->configDB);
                addShareUser.prepare(R"(
                    INSERT OR IGNORE INTO `User_Shares`(`User_Key`, `Share_Key`, `Default_Perms`, `Updt_User`)
                    SELECT `User_Key`,
                           ? as `Share_Key`,
                           ? as `Default_Perms`,
                           `Updt_User`
                    FROM `Users`
                    WHERE `User_Key` = ?;)");
                addShareUser.addBindValue(shareKey);
                addShareUser.addBindValue(i.value());
                addShareUser.addBindValue(i.key());
                if (addShareUser.exec() == false) {
                    Log::error(Log::Categories["Web Server"], "Error while adding a share user: {0}", ToChr(addShareUser.lastError().text()));
                    ph7_context_throw_error(pCtx, PH7_CTX_ERR, "Internal error while adding a share user.");
                    return PH7_OK;
                }
            }
            ph7_result_string(pCtx, "OK", -1);
        } else {
            ph7_result_string(pCtx, "You do not have permission to edit Shares.", -1);
            return PH7_OK;
        }
    } else {
        ph7_context_throw_error(pCtx, PH7_CTX_ERR, "You have specified an invalid number or type of arguments to this function.");
        return PH7_OK;
    }

    return PH7_OK;
}

static int PH7GetUsers(ph7_context *pCtx,int argc,ph7_value **argv) {
    SSNFSWorker *worker = (SSNFSWorker*)ph7_context_user_data(pCtx);

    QSqlQuery getUsers(worker->configDB);
    if (argc == 1) {
        if (ph7_value_is_int(argv[0])) {
            getUsers.prepare(R"(
                SELECT `User_Key`,
                       `FullName`,
                       `Email`,
                       CAST(strftime('%s',`Updt_TmStmp`) AS INTEGER),
                       `Updt_User`
                FROM `Users`
                WHERE `User_Key` = ?;)");
            getUsers.addBindValue(ph7_value_to_int64(argv[0]));
        } else if (ph7_value_is_string(argv[0])) {
            getUsers.prepare(R"(
                SELECT `User_Key`,
                       `FullName`,
                       `Email`,
                       CAST(strftime('%s',`Updt_TmStmp`) AS INTEGER),
                       `Updt_User`
                FROM `Users`
                WHERE `Email` = ?;)");
            getUsers.addBindValue(QString(ph7_value_to_string(argv[0], NULL)));
        } else {
            ph7_context_throw_error(pCtx, PH7_CTX_ERR, "If you specify an argument to this function, it must be either an integer or a string.");
            return PH7_OK;
        }
    } else if (argc == 0) {
        getUsers.prepare(R"(
            SELECT `User_Key`,
                   `FullName`,
                   `Email`,
                   CAST(strftime('%s',`Updt_TmStmp`) AS INTEGER),
                   `Updt_User`
            FROM `Users`;)");
    } else {
        ph7_context_throw_error(pCtx, PH7_CTX_ERR, "You have specified an invalid number of arguments to this function.");
        return PH7_OK;
    }

    if (getUsers.exec()) {
        ph7_value *returnVal = ph7_context_new_array(pCtx);
        while (getUsers.next()) {
            ph7_value *currUser = ph7_context_new_array(pCtx);
            ph7_value *userKeyVal = ph7_context_new_scalar(pCtx);
            ph7_value_int64(userKeyVal, getUsers.value(0).toLongLong());
            ph7_array_add_strkey_elem(currUser, "userKey", userKeyVal);
            ph7_value *userNameVal = ph7_context_new_scalar(pCtx);
            ph7_value_string(userNameVal, ToChr(getUsers.value(1).toString()), -1);
            ph7_array_add_strkey_elem(currUser, "userName", userNameVal);
            ph7_value *emailVal = ph7_context_new_scalar(pCtx);
            ph7_value_string(emailVal, ToChr(getUsers.value(2).toString()), -1);
            ph7_array_add_strkey_elem(currUser, "email", emailVal);
            ph7_value *updtTmStmpVal = ph7_context_new_scalar(pCtx);
            ph7_value_int64(updtTmStmpVal, getUsers.value(3).toLongLong());
            ph7_array_add_strkey_elem(currUser, "updtTmStmp", updtTmStmpVal);
            ph7_value *updtUserVal = ph7_context_new_scalar(pCtx);
            ph7_value_string(updtUserVal, ToChr(getUsers.value(4).toString()), -1);
            ph7_array_add_strkey_elem(currUser, "updtUser", updtUserVal);
            ph7_array_add_elem(returnVal, NULL, currUser);
        }
        ph7_result_value(pCtx, returnVal);
    } else {
        Log::error(Log::Categories["Web Server"], "Error while retrieving users: {0}", ToChr(getUsers.lastError().text()));
        ph7_context_throw_error(pCtx, PH7_CTX_ERR, "Internal error while retrieving users.");
        qInfo() << getUsers.executedQuery();
    }

    return PH7_OK;
}

// TODO: Read this from file?
#define HTTP_500_RESPONSE "HTTP/1.1 500 Internal Server Error\r\n" \
                          "Connection: close\r\n\r\n" \
                          "<html><body>\n" \
                          "<h3>An error occured while processing your request.</h3>\n" \
                          "This error has been logged and will be investigated shortly.\n" \
                          "</body></html>"
void SSNFSWorker::processHttpRequest(char firstChar)
{
    // Before we even do anything, delete Web Tokens which have not been accessed in the last 30 min.
    QSqlQuery delOldTokens(configDB);
    delOldTokens.prepare("DELETE FROM `Web_Tokens` WHERE ((julianday('now') - julianday(LastAccess_TmStmp)) * 24 * 60) > 30;");
    if (delOldTokens.exec() == false) {
        Log::error(Log::Categories["Web Server"], "Error removing old web authentication tokens: {0}", ToChr(delOldTokens.lastError().text()));
    }

    QByteArray Request;
    Request.append(firstChar);

    while (socket->canReadLine() == false) {
        socket->waitForReadyRead(-1);
    }
    QString RequestLine = socket->readLine();
    Request.append(RequestLine);
    QString RequestPath = RequestLine.split(" ")[1];
    RequestPath = RequestPath.split('?')[0];

    QString FinalPath = ServerSettings::get("WebPanelPath");
    FinalPath.append(Common::resolveRelative(RequestPath));

    QFileInfo FileFI(FinalPath);
    if (!FileFI.exists()) {
        socket->write("HTTP/1.1 404 Not Found\r\n");
        socket->write("Content-Type: text/html\r\n");
        socket->write("Connection: close\r\n\r\n");
        socket->write("<html><body><h3>The file or directory you requested does not exist.</h3></body></html>");
        return;
    }
    if (FileFI.isDir()) {
        if (!FinalPath.endsWith('/'))
            FinalPath.append('/');
        FinalPath.append("index.php");
    }
    FileFI.setFile(FinalPath);
    if (!FileFI.exists()) {
        socket->write("HTTP/1.1 404 Not Found\r\n");
        socket->write("Content-Type: text/html\r\n");
        socket->write("Connection: close\r\n\r\n");
        socket->write("<html><body><h3>The file or directory you requested does not exist.</h3></body></html>");
        return;
    }

    if (!FileFI.isReadable()) {
        socket->write("HTTP/1.1 403 Forbidden\r\n");
        socket->write("Content-Type: text/html\r\n");
        socket->write("Connection: close\r\n\r\n");
        socket->write("<html><body><h3>The file or directory you requested cannot be opened.</h3></body></html>");
        return;
    }

    QMap<QString, QString> Headers;
    while (true) {
        if (!socket->isOpen() || !socket->isEncrypted()) {
            return;
        }
        while (socket->canReadLine() == false) {
            if (!socket->waitForReadyRead(3000))
                continue;
        }
        QString HeaderLine = socket->readLine();
        Request.append(HeaderLine);
        HeaderLine = HeaderLine.trimmed();
        if (HeaderLine.isEmpty()) {
            break;
        }
        int HeaderNameLen = HeaderLine.indexOf(": ");
        Headers.insert(HeaderLine.mid(0, HeaderNameLen), HeaderLine.mid(HeaderNameLen + 2));
    }

    if (Headers.keys().contains("Content-Length")) {
        bool lengthOK;
        uint reqLength = Headers["Content-Length"].toUInt(&lengthOK);
        if (lengthOK) {
            while (reqLength > 0) {
                if (socket->bytesAvailable() == 0) {
                    if (!socket->waitForReadyRead(3000))
                        continue;
                }
                QByteArray currBatch = socket->readAll();
                reqLength -= currBatch.length();
                Request.append(currBatch);
            }
        }
    }

    if (FinalPath.endsWith(".php", Qt::CaseInsensitive)) {
        ph7 *pEngine; /* PH7 engine */
        ph7_vm *pVm; /* Compiled PHP program */
        int rc;
        /* Allocate a new PH7 engine instance */
        rc = ph7_init(&pEngine);
        if( rc != PH7_OK ){
            /*
            * If the supplied memory subsystem is so sick that we are unable
            * to allocate a tiny chunk of memory, there is not much we can do here.
            */
            Log::error(Log::Categories["Web Server"], "Error while allocating a new PH7 engine instance");
            socket->write(HTTP_500_RESPONSE);
            return;
        }
        rc = ph7_compile_file(
                    pEngine,
                    ToChr(FinalPath),
                    &pVm,
                    0);
        if( rc != PH7_OK ){
            if( rc == PH7_COMPILE_ERR ){
                const char *zErrLog;
                int nLen;
                /* Extract error log */
                ph7_config(pEngine,
                           PH7_CONFIG_ERR_LOG,
                           &zErrLog,
                           &nLen
                           );
                if( nLen > 0 ){
                    /* zErrLog is null terminated */
                    Log::error(Log::Categories["Web Server"], zErrLog);
                    socket->write(HTTP_500_RESPONSE);
                    return;
                }
            }
            Log::error(Log::Categories["Web Server"], "PH7: Unknown compile error.");
            socket->write(HTTP_500_RESPONSE);
            return;
        }
        /*
         * Now we have our script compiled, it's time to configure our VM.
         * We will install the output consumer callback defined above
         * so that we can consume and redirect the VM output to STDOUT.
         */
        rc = ph7_vm_config(pVm,
                           PH7_VM_CONFIG_OUTPUT,
                           PH7Consumer,
                           this /* Callback private data */
                           );
        if( rc != PH7_OK ){
            socket->write(HTTP_500_RESPONSE);
            Log::error(Log::Categories["Web Server"], "Error while installing the VM output consumer callback");
            return;
        }
        rc = ph7_vm_config(pVm,
                           PH7_VM_CONFIG_ERR_LOG_HANDLER,
                           PH7ErrorLogger
                           );
        if( rc != PH7_OK ){
            socket->write(HTTP_500_RESPONSE);
            Log::error(Log::Categories["Web Server"], "Error while installing the VM output consumer callback");
            return;
        }
        rc = ph7_vm_config(pVm,
                           PH7_VM_CONFIG_HTTP_REQUEST,
                           Request.data(),
                           Request.length()
                           );
        if( rc != PH7_OK ) {
            socket->write(HTTP_500_RESPONSE);
            Log::error(Log::Categories["Web Server"], "Error while transferring the HTTP request to PH7.");
            return;
        }
        QString WebPanelPath = ServerSettings::get("WebPanelPath");
        chdir(ToChr(WebPanelPath));

        rc = ph7_create_function(
                    pVm,
                    "header",
                    PH7SetHeader,
                    this);
        if( rc != PH7_OK ) {
            socket->write(HTTP_500_RESPONSE);
            Log::error(Log::Categories["Web Server"], "Error while setting up header() function in PH7.");
            return;
        }
        rc = ph7_create_function(
                    pVm,
                    "http_response_code",
                    PH7ResponseCode,
                    this);
        if( rc != PH7_OK ) {
            socket->write(HTTP_500_RESPONSE);
            Log::error(Log::Categories["Web Server"], "Error while setting up http_response_code() function in PH7.");
            return;
        }
        rc = ph7_create_function(
                    pVm,
                    "check_auth_cookie",
                    PH7CheckAuthCookie,
                    this);
        if( rc != PH7_OK ) {
            socket->write(HTTP_500_RESPONSE);
            Log::error(Log::Categories["Web Server"], "Error while setting up check_auth_cookie() function in PH7.");
            return;
        }
        rc = ph7_create_function(
                    pVm,
                    "make_auth_cookie",
                    PH7MakeAuthCookie,
                    this);
        if( rc != PH7_OK ) {
            socket->write(HTTP_500_RESPONSE);
            Log::error(Log::Categories["Web Server"], "Error while setting up make_auth_cookie() function in PH7.");
            return;
        }
        rc = ph7_create_function(
                    pVm,
                    "del_auth_cookie",
                    PH7DelAuthCookie,
                    this);
        if( rc != PH7_OK ) {
            socket->write(HTTP_500_RESPONSE);
            Log::error(Log::Categories["Web Server"], "Error while setting up del_auth_cookie() function in PH7.");
            return;
        }
        rc = ph7_create_function(
                    pVm,
                    "get_connected",
                    PH7GetConnected,
                    this);
        if( rc != PH7_OK ) {
            socket->write(HTTP_500_RESPONSE);
            Log::error(Log::Categories["Web Server"], "Error while setting up get_connected() function in PH7.");
            return;
        }
        rc = ph7_create_function(
                    pVm,
                    "get_pending",
                    PH7GetPending,
                    this);
        if( rc != PH7_OK ) {
            socket->write(HTTP_500_RESPONSE);
            Log::error(Log::Categories["Web Server"], "Error while setting up get_pending() function in PH7.");
            return;
        }
        rc = ph7_create_function(
                    pVm,
                    "approve_pending",
                    PH7ApprovePending,
                    this);
        if( rc != PH7_OK ) {
            socket->write(HTTP_500_RESPONSE);
            Log::error(Log::Categories["Web Server"], "Error while setting up approve_pending() function in PH7.");
            return;
        }
        rc = ph7_create_function(
                    pVm,
                    "check_perm",
                    PH7CheckPerm,
                    this);
        if( rc != PH7_OK ) {
            socket->write(HTTP_500_RESPONSE);
            Log::error(Log::Categories["Web Server"], "Error while setting up check_perm() function in PH7.");
            return;
        }
        rc = ph7_create_function(
                    pVm,
                    "get_shares",
                    PH7GetShares,
                    this);
        if( rc != PH7_OK ) {
            socket->write(HTTP_500_RESPONSE);
            Log::error(Log::Categories["Web Server"], "Error while setting up get_shares() function in PH7.");
            return;
        }
        rc = ph7_create_function(
                    pVm,
                    "save_share",
                    PH7SaveShare,
                    this);
        if( rc != PH7_OK ) {
            socket->write(HTTP_500_RESPONSE);
            Log::error(Log::Categories["Web Server"], "Error while setting up save_share() function in PH7.");
            return;
        }
        rc = ph7_create_function(
                    pVm,
                    "get_users",
                    PH7GetUsers,
                    this);
        if( rc != PH7_OK ) {
            socket->write(HTTP_500_RESPONSE);
            Log::error(Log::Categories["Web Server"], "Error while setting up get_users() function in PH7.");
            return;
        }

        /*
        * And finally, execute our program.
        */
        if (ph7_vm_exec(pVm,0) != PH7_OK) {
            socket->write(HTTP_500_RESPONSE);
            Log::error(Log::Categories["Web Server"], "Error occurred while running PH7 script for script '{0}'.");
            return;
        }
        /* All done, cleanup the mess left behind.
        */
        ph7_vm_release(pVm);
        ph7_release(pEngine);

        // Force Connection header to close since implementing keep-alive is not planned.
        responseHeaders["Connection"] = "close";
        socket->write(tr("HTTP/1.1 %1 %2\r\n").arg(httpResultCode).arg(knownResultCodes.value(httpResultCode)).toUtf8());
        for (QHash<QString, QString>::iterator i = responseHeaders.begin(); i != responseHeaders.end(); ++i) {
            socket->write(i.key().toUtf8());
            socket->write(": ");
            socket->write(i.value().toUtf8());
            socket->write("\r\n");
        }
        socket->write("\r\n");
        socket->write(httpResponse);
    } else {
        socket->write("HTTP/1.1 200 OK\r\n");
        QMimeDatabase mimeDB;
        socket->write(ToChr(tr("Content-Type: %1\r\n").arg(mimeDB.mimeTypeForFile(FinalPath).name())));
        // Let the browser know we plan to close this conenction.
        socket->write("Connection: close\r\n\r\n");
        QFile requestedFile(FinalPath);
        requestedFile.open(QFile::ReadOnly);
        while (!requestedFile.atEnd()) {
            socket->write(requestedFile.readAll());
        }
        requestedFile.close();
        socket->waitForBytesWritten(-1);
    }
}
