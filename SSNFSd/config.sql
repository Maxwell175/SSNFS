PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE "Settings" (
	`Setting_Key`	TEXT NOT NULL UNIQUE,
	`Setting_Value`	TEXT NOT NULL,
	`Setting_Desc`	TEXT NOT NULL,
	`Updt_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
	`Updt_User`	TEXT NOT NULL,
	PRIMARY KEY(`Setting_Key`)
);
INSERT INTO "Settings" VALUES('PrivateKeySource','file','Where is the private key?','2018-02-19 04:25:58','Default');
INSERT INTO "Settings" VALUES('PrivateKeyFilePath','/home/maxwell/Coding/SSNFS/SSNFSd/test.key','If the private key is a file, this is the path to the file.','2018-02-19 04:25:58','Default');
INSERT INTO "Settings" VALUES('CertificatePath','/home/maxwell/Coding/SSNFS/SSNFSd/test.crt','Path to the server''s certificate.','2018-02-19 04:25:58','Default');
INSERT INTO "Settings" VALUES('ListenPort','2050','The port on which the server will listen for both client requests and Web Panel requests.','2018-03-05 07:18:50','Default');
INSERT INTO "Settings" VALUES('NewFileMode','777','The mode used to store the files that clients create. Does not affect actual client privileges.','2018-04-06 03:33:34','Default');
INSERT INTO "Settings" VALUES('WebPanelPath','/home/maxwell/Coding/SSNFS/SSNFSd/webpanel','Path to the directory containing the web administration files. Before loading each file, it goes through a PHP parser.','2018-05-10 00:00:00','Default');
CREATE TABLE "Shares" (
	`Share_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	`Share_Name`	TEXT NOT NULL UNIQUE,
	`Local_Path`	TEXT NOT NULL,
	`Updt_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
	`Updt_User`	TEXT NOT NULL
);
INSERT INTO "Shares" VALUES(1,'TestShare','/home/maxwell/fuse-test-base','2018-02-19 04:47:22','2018-02-19 04:47:22');
CREATE TABLE "Logs" (
	`Log_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	`Log_Name`	TEXT NOT NULL UNIQUE,
	`Log_Path`	TEXT NOT NULL UNIQUE,
	`Updt_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
	`Updt_User`	TEXT NOT NULL
);
INSERT INTO "Logs" VALUES(1,'Default','stdout:///','2018-02-28 06:17:00','Default');
CREATE TABLE "Logs_Log_Categories" (
	`Log_Key`	INTEGER NOT NULL,
	`Log_Category`	TEXT NOT NULL,
	`Log_Levels`	TEXT NOT NULL,
	FOREIGN KEY(`Log_Key`) REFERENCES `Logs`(`Log_Key`)
);
INSERT INTO "Logs_Log_Categories" VALUES(1,'Connection','error,warn,info');
INSERT INTO "Logs_Log_Categories" VALUES(1,'Authentication','error,warn,info');
INSERT INTO "Logs_Log_Categories" VALUES(1,'File System','error,warn,info');
INSERT INTO "Logs_Log_Categories" VALUES(1,'Core','error,warn,info');
INSERT INTO "Logs_Log_Categories" VALUES(1,'Web Server','error,warn,info');
CREATE TABLE "Web_Tokens" (
	`User_Key`	INTEGER NOT NULL,
	`Token`	TEXT NOT NULL UNIQUE,
	`LastAccess_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
	FOREIGN KEY(`User_Key`) REFERENCES `Users`(`User_Key`) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE TABLE "Server_Roles" (
	`Role_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	`Role_Name`	TEXT NOT NULL UNIQUE
);
INSERT INTO "Server_Roles" VALUES(1,'User');
INSERT INTO "Server_Roles" VALUES(2,'Owner');
CREATE TABLE "Server_Perms" (
	`Perm_ShortName`	TEXT NOT NULL UNIQUE,
	`Perm_Name`	TEXT NOT NULL UNIQUE,
	`Perm_Desc`	TEXT NOT NULL UNIQUE,
	PRIMARY KEY(`Perm_ShortName`)
);
INSERT INTO "Server_Perms" VALUES('settings','Global Settings','View and change server settings.');
INSERT INTO "Server_Perms" VALUES('shares','Shares','View and change the settings of all shares.');
INSERT INTO "Server_Perms" VALUES('users','Users','View and change the settings for all users.');
INSERT INTO "Server_Perms" VALUES('computers','Computers','Approve and remove computers linked to any user.');
INSERT INTO "Server_Perms" VALUES('logs','Logs','Manage log outputs.');
INSERT INTO "Server_Perms" VALUES('connected','Connected Computers','View all connected computers.');
CREATE TABLE "User_Shares" (
	`User_Share_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	`User_Key`	INTEGER NOT NULL,
	`Share_Key`	INTEGER NOT NULL,
	`Default_Perms`	TEXT NOT NULL DEFAULT 'rwxo',
	`Updt_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
	`Updt_User`	TEXT NOT NULL,
	FOREIGN KEY(`Share_Key`) REFERENCES `Shares`(`Share_Key`) ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY(`User_Key`) REFERENCES `Users`(`User_Key`) ON DELETE CASCADE ON UPDATE CASCADE
);
INSERT INTO "User_Shares" VALUES(1,1,1,'rwxo','2018-03-19 16:42:14','Default');
CREATE TABLE "User_Share_Perms" (
	`Share_Key`	INTEGER NOT NULL,
	`User_Key`	INTEGER NOT NULL,
	`Client_User_Key`	INTEGER,
	`Path`	TEXT NOT NULL,
	`Perms`	TEXT NOT NULL DEFAULT 'rwxo',
	UNIQUE(`Share_Key`,`User_Key`,`Client_User_Key`,`Path`),
	FOREIGN KEY(`Share_Key`) REFERENCES `Shares`(`Share_Key`) ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY(`User_Key`) REFERENCES `Users`(`User_Key`) ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY(`Client_User_Key`) REFERENCES `Client_Users`(`Client_User_Key`) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE TABLE "Client_Users" (
	`Client_User_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	`Client_Key`	INTEGER NOT NULL,
	`User_UID`	TEXT NOT NULL,
	`User_Name`	TEXT NOT NULL,
	UNIQUE(`Client_Key`,`User_UID`),
	FOREIGN KEY(`Client_Key`) REFERENCES `Clients`(`Client_Key`) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE TABLE "Server_Role_Perms" (
	`Role_Key`	INTEGER NOT NULL,
	`Perm_ShortName`	TEXT NOT NULL,
	FOREIGN KEY(`Perm_ShortName`) REFERENCES `Server_Perms`(`Perm_ShortName`) ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY(`Role_Key`) REFERENCES `Server_Roles`(`Role_Key`) ON DELETE CASCADE ON UPDATE CASCADE
);
INSERT INTO "Server_Role_Perms" VALUES(2,'computers');
INSERT INTO "Server_Role_Perms" VALUES(2,'shares');
INSERT INTO "Server_Role_Perms" VALUES(2,'settings');
INSERT INTO "Server_Role_Perms" VALUES(2,'connected');
INSERT INTO "Server_Role_Perms" VALUES(2,'users');
INSERT INTO "Server_Role_Perms" VALUES(2,'logs');
CREATE TABLE "Clients" (
	`User_Key`	INTEGER NOT NULL,
	`Client_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	`Client_Name`	TEXT NOT NULL UNIQUE,
	`Client_Cert`	TEXT NOT NULL UNIQUE,
	`Client_Info`	TEXT NOT NULL UNIQUE,
	`Updt_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
	`Updt_User`	TEXT NOT NULL,
	FOREIGN KEY(`User_Key`) REFERENCES `Users`(`User_Key`)
);
INSERT INTO "Clients" VALUES(1,1,'MainPC','-----BEGIN CERTIFICATE-----
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
-----END CERTIFICATE-----
','Maxwell-Ubu-Main|ASUSTeK COMPUTER INC.|Z97-PRO GAMER|GenuineIntel|6|60|Intel(R) Core(TM) i7-4790K CPU @ 4.00GHz|4','2018-03-19 16:42:14','Default');
CREATE TABLE "Users" (
	`User_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	`FullName`	TEXT,
	`Email`	TEXT NOT NULL UNIQUE,
	`Password_Hash`	TEXT NOT NULL,
	`Role_Key`	INTEGER,
	`Updt_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
	`Crtd_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
	FOREIGN KEY(`Role_Key`) REFERENCES `Server_Roles`(`Role_Key`)
);
INSERT INTO "Users" VALUES(1,'Maxwell Dreytser','admin@mdtech.us','jbOieRVDBrbbKEvuqJ2bZaMjDdXKQPZ/gnj10QdxkZz3Z68yL+Zrz4roshvcu5KWLdRKQG0eQ2jrN58SN02d8Q==$4TWrYpBuOF8HSnaic4A7cmjRmdObAPCA$1024',2,'2018-05-12 03:44:02','2018-05-12 03:44:02');
DELETE FROM sqlite_sequence;
INSERT INTO "sqlite_sequence" VALUES('Shares',1);
INSERT INTO "sqlite_sequence" VALUES('Logs',1);
INSERT INTO "sqlite_sequence" VALUES('User_Shares',1);
INSERT INTO "sqlite_sequence" VALUES('Server_Roles',2);
INSERT INTO "sqlite_sequence" VALUES('Client_Users',0);
INSERT INTO "sqlite_sequence" VALUES('Clients',1);
INSERT INTO "sqlite_sequence" VALUES('Users',1);
CREATE UNIQUE INDEX `UQ_Perms` ON `User_Share_Perms` (
	`Share_Key`,
	`User_Key`,
	`Client_User_Key`,
	`Path`
);
CREATE UNIQUE INDEX `UQ_Client_Users` ON `Client_Users` (
	`Client_Key`,
	`User_UID`
);
COMMIT;
