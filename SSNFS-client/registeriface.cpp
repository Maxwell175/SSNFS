#include "registeriface.h"

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <openssl/pem.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <openssl/bio.h>

#include <QCoreApplication>
#include <QThread>
#include <QRegularExpression>

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)
#define HELLO_STR "SSNFS client version " STR(_CLIENT_VERSION)

int getch() {
    int ch;
    struct termios t_old, t_new;

    tcgetattr(STDIN_FILENO, &t_old);
    t_new = t_old;
    t_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    return ch;
}

QString getpass(bool show_asterisk = true)
{
    const char BACKSPACE = '\b';
    const char RETURN = '\n';

    QString password;
    unsigned char ch = 0;

    while ((ch=getch()) != RETURN) {
        if (ch == BACKSPACE) {
            if (password.length() != 0) {
                if (show_asterisk)
                    std::cout << "\b \b";
                password.resize(password.length() - 1);
            }
        } else {
            password += ch;
            if(show_asterisk)
                std::cout << '*';
        }
    }
    std::cout << std::endl;
    return password;
}

RegisterIface::RegisterIface(QObject *parent) : QObject(parent)
{
    QList<QString> paramParts = ((QString)qApp->arguments().at(2)).split(':');
    QString host = paramParts.at(0);
    bool portOK = true;
    quint16 port = (paramParts.length() == 2 ? paramParts.at(1).toUShort(&portOK) : 2050);
    if (!portOK) {
        qInfo() << "Invalid server port specified.";
        exit(1);
    }

    QTextStream ConsoleOut(stdout, QIODevice::WriteOnly);
    QTextStream ConsoleIn(stdin, QIODevice::ReadOnly);

    QString email;
    QByteArray shaPass;
    while (true) {
        ConsoleOut << "Enter your email: ";
        ConsoleOut.flush();
        email = ConsoleIn.readLine();
        if (QRegularExpression(".+@.+\\..+").match(email).hasMatch() == false) {
            ConsoleOut << "An invalid email was entered. Please try again.\n\n";
            continue;
        }

        ConsoleOut << "Password: ";
        ConsoleOut.flush();
        QString password = getpass();
        if (password.isEmpty()) {
            ConsoleOut << "Your password cannot be empty.\n\n";
            continue;
        }
        shaPass = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha512).toHex().toLower();
        password.clear();

        break;
    }

    ConsoleOut << "\nConnecting to Server... ";
    ConsoleOut.flush();

    int timeout = 10000;

    socket.setCaCertificates(QSslSocket::systemCaCertificates());
    socket.setPeerVerifyMode(QSslSocket::VerifyNone);

    socket.connectToHostEncrypted(host, port);

    if (socket.waitForEncrypted(timeout) == false) {
        qInfo() << "Failed to initiate server connection:" << socket.errorString();
        exit(1);
        return;
    }

    socket.write(Common::getBytes(Common::Register));
    if (socket.waitForBytesWritten(timeout) == false) {
        qInfo() << "Failed to complete server handshake:" << socket.errorString();
        exit(1);
        return;
    }

    if (socket.waitForReadyRead(timeout) == false) {
        qInfo() << "Failed to complete server handshake:" << socket.errorString();
        exit(1);
        return;
    }
    if (Common::getResultFromBytes(Common::readExactBytes(&socket, 1)) != Common::OK) {
        if (socket.isOpen()) {
            socket.close();
            socket.waitForDisconnected(timeout / 10);
        }

        qInfo() << "Error connecting to server: Invalid message from server.";
        exit(1);
    }

    socket.write(Common::getBytes((qint32)email.length()));
    socket.write(email.toUtf8());
    socket.write(Common::getBytes((qint32)shaPass.length()));
    socket.write(shaPass);
    if (socket.waitForBytesWritten(timeout) == false) {
        qInfo() << "Failed to authenticate to the server:" << socket.errorString();
        exit(1);
        return;
    }

    if (socket.waitForReadyRead(timeout) == false) {
        qInfo() << "Failed to complete server handshake:" << socket.errorString();
        exit(1);
        return;
    }
    Common::ResultCode result = Common::getResultFromBytes(Common::readExactBytes(&socket, 1));
    switch (result) {
    case Common::OK:
        break;
    case Common::Error:
    {
        qint32 errorMsgLen = Common::getInt32FromBytes(Common::readExactBytes(&socket, 4));
        QByteArray errorMsg = Common::readExactBytes(&socket, errorMsgLen);
        ConsoleOut << "Failed.\n" << errorMsg;
        ConsoleOut.flush();
        exit(1);
    }
        break;
    default:
        if (socket.isOpen()) {
            socket.close();
            socket.waitForDisconnected(timeout / 10);
        }

        qInfo() << "Error connecting to server: Invalid message from server.";
        exit(1);

        break;
    }

    ConsoleOut << "DONE\n\n";

    QString clientName;
    while (true) {
        ConsoleOut << "Please specify a name for this computer: ";
        ConsoleOut.flush();
        clientName = ConsoleIn.readLine();
        if (clientName.isNull() || clientName.isEmpty())
            continue;
        else
            break;
    }

    if (QFile::exists(_CONFIG_DIR "/cert.pem") == false || QFile::exists(_CONFIG_DIR "/key.pem") == false) {
        ConsoleOut << "Generating a new certificate for this system... ";
        ConsoleOut.flush();

        int ret = 0;
        RSA *rsa = NULL;
        BIGNUM *bne = NULL;
        BIO *pkeywriter = NULL;
        BIO *certwriter = NULL;
        EVP_PKEY * pkey = NULL;

        int bits = 2048;
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
        pkeywriter = BIO_new_file(_CONFIG_DIR "/key.pem", "w");
        ret = PEM_write_bio_RSAPrivateKey(pkeywriter, rsa, NULL, NULL, 0, NULL, NULL);

        // Assign it.
        pkey = EVP_PKEY_new();
        EVP_PKEY_assign_RSA(pkey, rsa);

        // Generate X509.
        X509 * x509;
        x509 = X509_new();
        ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
        X509_gmtime_adj(X509_get_notBefore(x509), 0);
        X509_gmtime_adj(X509_get_notAfter(x509), 60L * 60L * 24L * 365L);
        X509_set_pubkey(x509, pkey);
        X509_NAME * name;
        name = X509_get_subject_name(x509);
        X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC,
                                   (unsigned char *)"  ", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC,
                                   (unsigned char *)"SSNFS", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "OU",  MBSTRING_ASC,
                                   (unsigned char *)"Client", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                                   (unsigned char *)ToChr(QHostInfo::localHostName()), -1, -1, 0);
        X509_set_issuer_name(x509, name);

        // Sign it.
        X509_sign(x509, pkey, EVP_sha1());

        // Write cert to file.
        certwriter = BIO_new_file(_CONFIG_DIR "/cert.pem", "w");
        PEM_write_bio_X509(certwriter, x509);

        // free
free_all:
        BIO_free_all(pkeywriter);
        BIO_free_all(certwriter);
        RSA_free(rsa);
        BN_free(bne);
        EVP_PKEY_free(pkey);

        if (ret != 1) {
            ConsoleOut << "Failed.\n" << ERR_error_string(ERR_get_error(), NULL);
            exit(1);
        } else {
            ConsoleOut << "Done\n";
            ConsoleOut.flush();
        }
    }

    QByteArray clientNameBytes(clientName.toUtf8());
    socket.write(Common::getBytes((qint32)clientNameBytes.length()));
    socket.write(clientNameBytes);
    QByteArray sysInfo = Common::getSystemInfo();
    socket.write(Common::getBytes((qint32)sysInfo.length()));
    socket.write(sysInfo);
    QFile certReader(_CONFIG_DIR "/cert.pem");
    if (certReader.open(QFile::ReadOnly) == false) {
        ConsoleOut << "Error reading certificate from file.";
        exit(1);
    }
    QSslCertificate cert(certReader.readAll());
    QByteArray certFingerprint = QCryptographicHash::hash(cert.toDer(), QCryptographicHash::Sha256).toHex().toLower();
    socket.write(Common::getBytes((qint32)certFingerprint.length()));
    socket.write(certFingerprint);
    certReader.close();

    if (socket.waitForBytesWritten(timeout) == false) {
        qInfo() << "Failed to send your certificate to the server:" << socket.errorString();
        exit(1);
        return;
    }

    if (socket.waitForReadyRead(timeout) == false) {
        qInfo() << "Failed to send your certificate to the server:" << socket.errorString();
        exit(1);
        return;
    }
    Common::ResultCode certResult = Common::getResultFromBytes(Common::readExactBytes(&socket, 1));
    switch (certResult) {
    case Common::OK:
        break;
    case Common::Error:
    {
        qint32 errorMsgLen = Common::getInt32FromBytes(Common::readExactBytes(&socket, 4));
        QByteArray errorMsg = Common::readExactBytes(&socket, errorMsgLen);
        qInfo() << errorMsg;
        exit(1);
    }
        break;
    default:
        if (socket.isOpen()) {
            socket.close();
            socket.waitForDisconnected(timeout / 10);
        }

        qInfo() << "Error sending your certificate to server: Invalid message from server.";
        exit(1);

        break;
    }

    qint32 successMsgLen = Common::getInt32FromBytes(Common::readExactBytes(&socket, 4));
    QByteArray successMsg = Common::readExactBytes(&socket, successMsgLen);

    ConsoleOut << "\n" << successMsg.data();
    ConsoleOut.flush();
}
