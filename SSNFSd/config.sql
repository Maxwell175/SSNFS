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
INSERT INTO Settings VALUES('PrivateKeySource','file','Where is the private key?','2018-02-19 04:25:58','Default');
INSERT INTO Settings VALUES('PrivateKeyFilePath','/home/maxwell/CLionProjects/SSNFS/SSNFSd/test.key','If the private key is a file, this is the path to the file.','2018-02-19 04:25:58','Default');
INSERT INTO Settings VALUES('CertificatePath','/home/maxwell/CLionProjects/SSNFS/SSNFSd/test.crt','Path to the server''s certificate.','2018-02-19 04:25:58','Default');
INSERT INTO Settings VALUES('ListenPort','2050','The port on which the server will listen for both client requests and Web Panel requests.','2018-03-05 07:18:50','Default');
INSERT INTO Settings VALUES('NewFileMode','777','The mode used to store the files that clients create. Does not affect actual client privileges.','2018-04-06 03:33:34','Default');
INSERT INTO Settings VALUES('WebPanelPath','/home/maxwell/CLionProjects/SSNFS/SSNFSd/webpanel','Path to the directory containing the web administration files. Before loading each file, it goes through a PHP parser.','2018-05-10 00:00:00','Default');
CREATE TABLE IF NOT EXISTS "Shares" (
	`Share_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	`Share_Name`	TEXT NOT NULL UNIQUE,
	`Local_Path`	TEXT NOT NULL,
	`Updt_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
	`Updt_User`	TEXT NOT NULL
);
INSERT INTO Shares VALUES(1,'TestShare','/home/maxwell/fuse-test-base','2018-02-19 04:47:22','2018-02-19 04:47:22');
CREATE TABLE IF NOT EXISTS "Logs" (
	`Log_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	`Log_Name`	TEXT NOT NULL UNIQUE,
	`Log_Path`	TEXT NOT NULL UNIQUE,
	`Updt_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
	`Updt_User`	TEXT NOT NULL
);
INSERT INTO Logs VALUES(1,'Default','stdout:///','2018-02-28 06:17:00','Default');
CREATE TABLE IF NOT EXISTS "Logs_Log_Categories" (
	`Log_Key`	INTEGER NOT NULL,
	`Log_Category`	TEXT NOT NULL,
	`Log_Levels`	TEXT NOT NULL,
	FOREIGN KEY(`Log_Key`) REFERENCES `Logs`(`Log_Key`)
);
INSERT INTO Logs_Log_Categories VALUES(1,'Connection','error,warn,info');
INSERT INTO Logs_Log_Categories VALUES(1,'Authentication','error,warn,info');
INSERT INTO Logs_Log_Categories VALUES(1,'File System','error,warn,info');
INSERT INTO Logs_Log_Categories VALUES(1,'Core','error,warn,info');
INSERT INTO Logs_Log_Categories VALUES(1,'Web Server','error,warn,info');
CREATE TABLE IF NOT EXISTS "Clients" (
	`Client_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	`Client_Name`	TEXT NOT NULL UNIQUE,
	`Updt_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
	`Updt_User`	TEXT NOT NULL
);
INSERT INTO Clients VALUES(1,'admin','2018-03-19 16:42:14','Default');
CREATE TABLE Client_Certs (Client_Key INTEGER NOT NULL, Client_Cert TEXT NOT NULL UNIQUE, Client_Info TEXT NOT NULL, FOREIGN KEY (Client_Key) REFERENCES Clients (Client_Key) ON DELETE CASCADE ON UPDATE CASCADE);
INSERT INTO Client_Certs VALUES(1,replace('-----BEGIN CERTIFICATE-----\n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\n-----END CERTIFICATE-----\n','\n',char(10)),'Maxwell-Ubu-Main|ASUSTeK COMPUTER INC.|Z97-PRO GAMER|GenuineIntel|6|60|Intel(R) Core(TM) i7-4790K CPU @ 4.00GHz|4');
CREATE TABLE Client_Shares (User_Share_Key INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE, Client_Key INTEGER NOT NULL REFERENCES Clients (Client_Key) ON DELETE CASCADE ON UPDATE CASCADE, Share_Key INTEGER NOT NULL, Default_Perms TEXT DEFAULT 'rwxo' NOT NULL, Updt_TmStmp INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP, Updt_User TEXT NOT NULL, FOREIGN KEY (Share_Key) REFERENCES Shares (Share_Key) ON DELETE CASCADE ON UPDATE CASCADE, FOREIGN KEY (Client_Key) REFERENCES Clients (Client_Key) ON DELETE CASCADE ON UPDATE CASCADE);
INSERT INTO Client_Shares VALUES(1,1,1,'rwxo','2018-03-19 16:42:14','Default');
CREATE TABLE Client_Users (Client_User_Key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE, Client_Key INTEGER NOT NULL, User_UID INTEGER NOT NULL, User_Name TEXT NOT NULL, FOREIGN KEY (Client_Key) REFERENCES Clients (Client_Key) ON DELETE CASCADE ON UPDATE CASCADE, UNIQUE (Client_Key, User_UID));
INSERT INTO Client_Users VALUES(117,1,0,'root');
INSERT INTO Client_Users VALUES(118,1,1,'daemon');
INSERT INTO Client_Users VALUES(119,1,2,'bin');
INSERT INTO Client_Users VALUES(120,1,3,'sys');
INSERT INTO Client_Users VALUES(121,1,4,'sync');
INSERT INTO Client_Users VALUES(122,1,5,'games');
INSERT INTO Client_Users VALUES(123,1,6,'man');
INSERT INTO Client_Users VALUES(124,1,7,'lp');
INSERT INTO Client_Users VALUES(125,1,8,'mail');
INSERT INTO Client_Users VALUES(126,1,9,'news');
INSERT INTO Client_Users VALUES(127,1,10,'uucp');
INSERT INTO Client_Users VALUES(128,1,13,'proxy');
INSERT INTO Client_Users VALUES(129,1,33,'www-data');
INSERT INTO Client_Users VALUES(130,1,34,'backup');
INSERT INTO Client_Users VALUES(131,1,38,'list');
INSERT INTO Client_Users VALUES(132,1,39,'irc');
INSERT INTO Client_Users VALUES(133,1,41,'gnats');
INSERT INTO Client_Users VALUES(134,1,65534,'nobody');
INSERT INTO Client_Users VALUES(135,1,100,'systemd-timesync');
INSERT INTO Client_Users VALUES(136,1,101,'systemd-network');
INSERT INTO Client_Users VALUES(137,1,102,'systemd-resolve');
INSERT INTO Client_Users VALUES(138,1,104,'syslog');
INSERT INTO Client_Users VALUES(139,1,105,'_apt');
INSERT INTO Client_Users VALUES(140,1,106,'messagebus');
INSERT INTO Client_Users VALUES(141,1,107,'uuidd');
INSERT INTO Client_Users VALUES(142,1,108,'rtkit');
INSERT INTO Client_Users VALUES(143,1,109,'avahi-autoipd');
INSERT INTO Client_Users VALUES(144,1,110,'dnsmasq');
INSERT INTO Client_Users VALUES(145,1,111,'usbmux');
INSERT INTO Client_Users VALUES(146,1,112,'whoopsie');
INSERT INTO Client_Users VALUES(147,1,113,'kernoops');
INSERT INTO Client_Users VALUES(148,1,114,'avahi');
INSERT INTO Client_Users VALUES(149,1,115,'pulse');
INSERT INTO Client_Users VALUES(150,1,116,'colord');
INSERT INTO Client_Users VALUES(151,1,117,'saned');
INSERT INTO Client_Users VALUES(152,1,118,'hplip');
INSERT INTO Client_Users VALUES(153,1,1000,'maxwell');
INSERT INTO Client_Users VALUES(154,1,121,'geoclue');
INSERT INTO Client_Users VALUES(155,1,122,'monotone');
INSERT INTO Client_Users VALUES(156,1,123,'sshd');
INSERT INTO Client_Users VALUES(157,1,124,'stunnel4');
INSERT INTO Client_Users VALUES(158,1,120,'nx');
INSERT INTO Client_Users VALUES(159,1,125,'trafficserver');
INSERT INTO Client_Users VALUES(160,1,126,'statd');
INSERT INTO Client_Users VALUES(161,1,1001,'main-backup');
INSERT INTO Client_Users VALUES(162,1,127,'usermetrics');
INSERT INTO Client_Users VALUES(163,1,128,'nm-openvpn');
INSERT INTO Client_Users VALUES(164,1,129,'lightdm');
INSERT INTO Client_Users VALUES(165,1,130,'speech-dispatcher');
INSERT INTO Client_Users VALUES(166,1,119,'sddm');
INSERT INTO Client_Users VALUES(167,1,132,'ice');
INSERT INTO Client_Users VALUES(168,1,133,'boinc');
INSERT INTO Client_Users VALUES(169,1,134,'opencvr');
INSERT INTO Client_Users VALUES(170,1,135,'ntp');
INSERT INTO Client_Users VALUES(171,1,131,'mpd');
INSERT INTO Client_Users VALUES(172,1,136,'gdm');
INSERT INTO Client_Users VALUES(173,1,137,'tftp');
INSERT INTO Client_Users VALUES(174,1,1002,'test');
CREATE TABLE User_Share_Perms (Share_Key INTEGER NOT NULL, Client_Key INTEGER NOT NULL REFERENCES Clients (Client_Key) ON DELETE CASCADE ON UPDATE CASCADE, Client_User_Key INTEGER, Path TEXT NOT NULL, Perms TEXT NOT NULL DEFAULT 'rwxo', FOREIGN KEY (Client_Key) REFERENCES Clients (Client_Key) ON DELETE CASCADE ON UPDATE CASCADE, FOREIGN KEY (Share_Key) REFERENCES Shares (Share_Key) ON DELETE CASCADE ON UPDATE CASCADE, FOREIGN KEY (Client_User_Key) REFERENCES Client_Users (Client_User_Key) ON DELETE CASCADE ON UPDATE CASCADE, UNIQUE (Share_Key, Client_Key, Client_User_Key, Path));
INSERT INTO User_Share_Perms VALUES(1,1,NULL,'/test/file','rxo');
INSERT INTO User_Share_Perms VALUES(1,1,174,'/test/file','rwxo');
CREATE TABLE IF NOT EXISTS "Users" (
	`User_Key`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	`Username`	TEXT NOT NULL UNIQUE,
	`Email`	TEXT NOT NULL UNIQUE,
	`Password_Hash`	TEXT NOT NULL,
	`Updt_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
	`Crtd_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE IF NOT EXISTS "Web_Tokens" (
	`User_Key`	INTEGER NOT NULL,
	`Token`	TEXT NOT NULL UNIQUE,
	`LastAccess_TmStmp`	INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,
	FOREIGN KEY(`User_Key`) REFERENCES `Users`(`User_Key`) ON DELETE CASCADE ON UPDATE CASCADE
);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('Shares',1);
INSERT INTO sqlite_sequence VALUES('Logs',1);
INSERT INTO sqlite_sequence VALUES('Clients',1);
INSERT INTO sqlite_sequence VALUES('Client_Shares',1);
INSERT INTO sqlite_sequence VALUES('Client_Users',3074);
INSERT INTO sqlite_sequence VALUES('Users',0);
CREATE UNIQUE INDEX UQ_Perms ON User_Share_Perms (Share_Key, Client_Key, Client_User_Key, Path);
COMMIT;
