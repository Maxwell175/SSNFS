PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS "Settings" (
        `Setting_Key`	TEXT NOT NULL UNIQUE,
        `Setting_Value`	TEXT NOT NULL,
        `Setting_Desc`	TEXT NOT NULL,
        `Updt_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
        `Updt_User`	TEXT NOT NULL,
        PRIMARY KEY(`Setting_Key`)
);
INSERT OR IGNORE INTO Settings VALUES('PrivateKeySource','file','Where is the private key?',CURRENT_TIMESTAMP,'Default');
INSERT OR IGNORE INTO Settings VALUES('PrivateKeyFilePath','','If the private key is a file, this is the path to the file.',CURRENT_TIMESTAMP,'Default');
INSERT OR IGNORE INTO Settings VALUES('CertificatePath','','Path to the server''s certificate.',CURRENT_TIMESTAMP,'Default');
INSERT OR IGNORE INTO Settings VALUES('ListenPort','2050','The port on which the server will listen for both client requests and Web Panel requests.',CURRENT_TIMESTAMP,'Default');
INSERT OR IGNORE INTO Settings VALUES('NewFileMode','777','The mode used to store the files that clients create. Does not affect actual client privileges.',CURRENT_TIMESTAMP,'Default');
INSERT OR IGNORE INTO Settings VALUES('WebPanelPath','','Path to the directory containing the web administration files. Before loading each file, it goes through a PHP parser.',CURRENT_TIMESTAMP,'Default');
INSERT OR IGNORE INTO Settings VALUES('UserCanApprove','true','Can a user approve clients without administrator involvement?',CURRENT_TIMESTAMP,'Default');
CREATE TABLE IF NOT EXISTS "Shares" (
        `Share_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
        `Share_Name`	TEXT NOT NULL UNIQUE,
        `Local_Path`	TEXT NOT NULL,
        `Updt_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
        `Updt_User`	TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS "Logs" (
        `Log_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
        `Log_Name`	TEXT NOT NULL UNIQUE,
        `Log_Path`	TEXT NOT NULL UNIQUE,
        `Updt_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
        `Updt_User`	TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS "Logs_Log_Categories" (
        `Log_Key`	INTEGER NOT NULL,
        `Log_Category`	TEXT NOT NULL,
        `Log_Levels`	TEXT NOT NULL,
        FOREIGN KEY(`Log_Key`) REFERENCES `Logs`(`Log_Key`)
);
CREATE UNIQUE INDEX IF NOT EXISTS `UQ_Logs_Log_Categories` ON `Logs_Log_Categories` (
        `Log_Key`,
        `Log_Category`
);
CREATE TABLE IF NOT EXISTS "Server_Roles" (
        `Role_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
        `Role_Name`	TEXT NOT NULL UNIQUE
);
INSERT OR IGNORE INTO Server_Roles(`Role_Name`) VALUES('User');
INSERT OR IGNORE INTO Server_Roles(`Role_Name`) VALUES('Administrator');
CREATE TABLE IF NOT EXISTS "Server_Perms" (
        `Perm_ShortName`	TEXT NOT NULL UNIQUE,
        `Perm_Name`	TEXT NOT NULL UNIQUE,
        `Perm_Desc`	TEXT NOT NULL UNIQUE,
        PRIMARY KEY(`Perm_ShortName`)
);
INSERT OR IGNORE INTO Server_Perms VALUES('settings','Global Settings','View and change server settings.');
INSERT OR IGNORE INTO Server_Perms VALUES('shares','Shares','View and change the settings of all shares.');
INSERT OR IGNORE INTO Server_Perms VALUES('users','Users','View and change the settings for all users.');
INSERT OR IGNORE INTO Server_Perms VALUES('computers','Computers','Approve and remove computers linked to any user.');
INSERT OR IGNORE INTO Server_Perms VALUES('logs','Logs','Manage log outputs.');
INSERT OR IGNORE INTO Server_Perms VALUES('connected','Connected Computers','View all connected computers.');
CREATE TABLE IF NOT EXISTS "Server_Role_Perms" (
        `Role_Key`	INTEGER NOT NULL,
        `Perm_ShortName`	TEXT NOT NULL,
        FOREIGN KEY(`Perm_ShortName`) REFERENCES `Server_Perms`(`Perm_ShortName`) ON DELETE CASCADE ON UPDATE CASCADE,
        FOREIGN KEY(`Role_Key`) REFERENCES `Server_Roles`(`Role_Key`) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE UNIQUE INDEX IF NOT EXISTS `UQ_Server_Role_Perms` ON `Server_Role_Perms` (
        `Role_Key`,
        `Perm_ShortName`
);
INSERT OR IGNORE INTO Server_Role_Perms
SELECT Role_Key, 'computers' as Perm_ShortName
FROM `Server_Roles`
WHERE Role_Name = 'Administrator';
INSERT OR IGNORE INTO Server_Role_Perms
SELECT Role_Key, 'shares' as Perm_ShortName
FROM `Server_Roles`
WHERE Role_Name = 'Administrator';
INSERT OR IGNORE INTO Server_Role_Perms
SELECT Role_Key, 'settings' as Perm_ShortName
FROM `Server_Roles`
WHERE Role_Name = 'Administrator';
INSERT OR IGNORE INTO Server_Role_Perms
SELECT Role_Key, 'connected' as Perm_ShortName
FROM `Server_Roles`
WHERE Role_Name = 'Administrator';
INSERT OR IGNORE INTO Server_Role_Perms
SELECT Role_Key, 'users' as Perm_ShortName
FROM `Server_Roles`
WHERE Role_Name = 'Administrator';
INSERT OR IGNORE INTO Server_Role_Perms
SELECT Role_Key, 'logs' as Perm_ShortName
FROM `Server_Roles`
WHERE Role_Name = 'Administrator';
CREATE TABLE IF NOT EXISTS "Users" (
        `User_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
        `FullName`	TEXT,
        `Email`	TEXT NOT NULL UNIQUE,
        `Password_Hash`	TEXT NOT NULL,
        `Updt_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
        `Crtd_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE IF NOT EXISTS "User_Roles" (
        `User_Key`  INTEGER NOT NULL,
        `Role_Key`  INTEGER NOT NULL,
        FOREIGN KEY(`User_Key`) REFERENCES `Users`(`User_Key`) ON DELETE CASCADE ON UPDATE CASCADE,
        FOREIGN KEY(`Role_Key`) REFERENCES `Server_Roles`(`Role_Key`) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE UNIQUE INDEX IF NOT EXISTS `UQ_User_Roles` ON `User_Roles` (
        `User_Key`,
        `Role_Key`
);
CREATE TABLE IF NOT EXISTS "User_Shares" (
        `User_Share_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
        `User_Key`	INTEGER NOT NULL,
        `Share_Key`	INTEGER NOT NULL,
        `Default_Perms`	TEXT NOT NULL,
        `Updt_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
        `Updt_User`	TEXT NOT NULL,
        FOREIGN KEY(`Share_Key`) REFERENCES `Shares`(`Share_Key`) ON DELETE CASCADE ON UPDATE CASCADE,
        FOREIGN KEY(`User_Key`) REFERENCES `Users`(`User_Key`) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE UNIQUE INDEX IF NOT EXISTS `UQ_User_Shares` ON `User_Shares` (
        `User_Key`,
        `Share_Key`
);
CREATE TABLE IF NOT EXISTS "User_Share_Perms" (
        `Share_Key`	INTEGER NOT NULL,
        `User_Key`	INTEGER NOT NULL,
        `Client_User_Key`	INTEGER,
        `Path`	TEXT NOT NULL,
        `Perms`	TEXT NOT NULL,
        UNIQUE(`Share_Key`,`User_Key`,`Client_User_Key`,`Path`),
        FOREIGN KEY(`Share_Key`) REFERENCES `Shares`(`Share_Key`) ON DELETE CASCADE ON UPDATE CASCADE,
        FOREIGN KEY(`User_Key`) REFERENCES `Users`(`User_Key`) ON DELETE CASCADE ON UPDATE CASCADE,
        FOREIGN KEY(`Client_User_Key`) REFERENCES `Client_Users`(`Client_User_Key`) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE UNIQUE INDEX IF NOT EXISTS `UQ_Perms` ON `User_Share_Perms` (
        `Share_Key`,
        `User_Key`,
        `Client_User_Key`,
        `Path`
);
CREATE TABLE IF NOT EXISTS "Clients" (
        `Client_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
        `User_Key`	INTEGER NOT NULL,
        `Client_Name`	TEXT NOT NULL,
        `Client_Cert`	TEXT NOT NULL,
        `Client_Info`	TEXT NOT NULL,
        `Updt_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
        `Updt_User`	TEXT NOT NULL,
        FOREIGN KEY(`User_Key`) REFERENCES `Users`(`User_Key`)
);
CREATE UNIQUE INDEX IF NOT EXISTS `UQ_Clients` ON `Clients` (
        `User_Key`,
        `Client_Cert`
);
CREATE UNIQUE INDEX IF NOT EXISTS `UQ_Client_Name_User` ON `Clients` (
        `User_Key`,
        `Client_Name`
);
CREATE TABLE IF NOT EXISTS "Pending_Clients" (
        `Pending_Client_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
        `User_Key`	INTEGER NOT NULL,
        `Client_Name`	TEXT NOT NULL,
        `Client_Cert`	TEXT NOT NULL,
        `Client_Info`	TEXT NOT NULL,
        `Submit_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
        `Submit_Host`	TEXT NOT NULL,
        FOREIGN KEY(`User_Key`) REFERENCES `Users`(`User_Key`)
);
CREATE UNIQUE INDEX IF NOT EXISTS `UQ_Pending_Clients` ON `Pending_Clients` (
        `User_Key`,
        `Client_Cert`
);
CREATE TABLE IF NOT EXISTS "Client_Users" (
        `Client_User_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
        `Client_Key`	INTEGER NOT NULL,
        `User_UID`	TEXT NOT NULL,
        `User_Name`	TEXT NOT NULL,
        UNIQUE(`Client_Key`,`User_UID`),
        FOREIGN KEY(`Client_Key`) REFERENCES `Clients`(`Client_Key`) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE UNIQUE INDEX IF NOT EXISTS `UQ_Client_Users` ON `Client_Users` (
        `Client_Key`,
        `User_UID`
);
CREATE TABLE IF NOT EXISTS "Web_Tokens" (
        `User_Key`	INTEGER NOT NULL,
        `Token`	TEXT NOT NULL UNIQUE,
        `LastAccess_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY(`User_Key`) REFERENCES `Users`(`User_Key`) ON DELETE CASCADE ON UPDATE CASCADE
);

COMMIT;

PRAGMA foreign_keys=ON;
