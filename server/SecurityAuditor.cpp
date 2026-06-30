#include "SecurityAuditor.h"
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslCertificate>

namespace SecurityAudit {

SecurityAuditor::SecurityAuditor(QObject *parent)
    : QObject(parent)
    , socket(nullptr)
    , sslSocket(nullptr)
    , timer(nullptr)
    , currentPort(0)
    , currentResult(nullptr)
{
}

void SecurityAuditor::auditService(const QString& host, int port, ScanResult& result) {
    currentPort = port;
    currentResult = &result;
    
    // Определяем тип сервиса и запускаем соответствующую проверку
    if (port == 80 || port == 8080) {
        checkHTTP(host, port, result);
    } else if (port == 443 || port == 8443) {
        checkHTTPS(host, port, result);
    } else if (port == 22) {
        checkSSH(host, port, result);
    } else if (port == 21) {
        checkFTP(host, port, result);
    } else if (port == 25) {
        checkSMTP(host, port, result);
    } else if (port == 3306) {
        checkMySQL(host, port, result);
    } else {
        // Для остальных портов просто получаем баннер
        result.banner = grabBanner(host, port);
    }
}

void SecurityAuditor::checkHTTP(const QString& host, int port, ScanResult& result) {
    // Проверяем HTTP
    socket = new QTcpSocket(this);
    socket->connectToHost(host, port);
    
    if (socket->waitForConnected(3000)) {
        // Отправляем HTTP запрос
        QByteArray request = "HEAD / HTTP/1.1\r\nHost: " + host.toUtf8() + "\r\nConnection: close\r\n\r\n";
        socket->write(request);
        socket->waitForReadyRead(3000);
        
        QByteArray response = socket->readAll();
        QString responseStr = QString::fromUtf8(response);
        
        // Извлекаем заголовок Server
        for (const QString& line : responseStr.split("\r\n")) {
            if (line.toLower().startsWith("server:")) {
                result.httpServer = line.mid(7).trimmed();
            }
        }
        
        // Проверяем редирект на HTTPS
        if (responseStr.contains("301") || responseStr.contains("302")) {
            if (responseStr.toLower().contains("location: https")) {
                result.hasHttpsRedirect = true;
            }
        }
        
        // Проверяем заголовки безопасности
        QStringList requiredHeaders = {
            "Strict-Transport-Security",
            "Content-Security-Policy",
            "X-Frame-Options",
            "X-Content-Type-Options",
            "Referrer-Policy"
        };
        
        for (const QString& line : responseStr.split("\r\n")) {
            for (const QString& header : requiredHeaders) {
                if (line.toLower().startsWith(header.toLower() + ":")) {
                    result.securityHeaders.append(line.trimmed());
                }
            }
        }
        
        // Проверяем HTTP методы
        socket->write("OPTIONS / HTTP/1.1\r\nHost: " + host.toUtf8() + "\r\n\r\n");
        socket->waitForReadyRead(1000);
        QByteArray optionsResponse = socket->readAll();
        QString optionsStr = QString::fromUtf8(optionsResponse);
        
        for (const QString& line : optionsStr.split("\r\n")) {
            if (line.toLower().startsWith("allow:")) {
                QString methods = line.mid(6).trimmed();
                result.httpMethods = methods.split(", ");
            }
        }
        
        // Проверяем уязвимости
        if (result.httpServer.contains("Apache/2.2")) {
            result.vulnerable = true;
            result.vulnerability = "Устаревшая версия Apache (2.2) - множество известных уязвимостей";
        }
        if (result.httpServer.contains("nginx/1.") && 
            result.httpServer.mid(result.httpServer.indexOf("nginx/")+6, 3).toFloat() < 1.18) {
            result.vulnerable = true;
            result.vulnerability = "Устаревшая версия Nginx - уязвимость CVE-2021-23017";
        }
        if (result.securityHeaders.size() < 3) {
            result.vulnerable = true;
            result.vulnerability = "Отсутствуют важные заголовки безопасности (защита от XSS, Clickjacking)";
        }
        
        result.banner = responseStr.left(200);
        socket->close();
    }
    socket->deleteLater();
}

void SecurityAuditor::checkHTTPS(const QString& host, int port, ScanResult& result) {
    // Проверяем HTTPS и SSL сертификат
    sslSocket = new QSslSocket(this);
    sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
    sslSocket->connectToHostEncrypted(host, port);
    
    if (sslSocket->waitForEncrypted(5000)) {
        // Получаем информацию о SSL
        QSslConfiguration sslConfig = sslSocket->sslConfiguration();
        result.sslVersion = sslConfig.sessionProtocolString();
        result.sslCipher = sslConfig.sessionCipher().name();
        
        // Проверяем сертификат
        QSslCertificate cert = sslSocket->peerCertificate();
        if (!cert.isNull()) {
            result.sslExpired = (cert.expiryDate() < QDateTime::currentDateTime());
            
            if (result.sslExpired) {
                result.vulnerable = true;
                result.vulnerability = "SSL сертификат просрочен!";
            }
        }
        
        // Проверяем уязвимости SSL
        if (result.sslVersion.contains("TLSv1.0") || result.sslVersion.contains("TLSv1.1")) {
            result.vulnerable = true;
            result.vulnerability = "Используется устаревшая версия " + result.sslVersion + " (уязвимость POODLE/BEAST)";
        }
        if (result.sslVersion.contains("SSLv")) {
            result.vulnerable = true;
            result.vulnerability = "Используется небезопасный протокол SSL!";
        }
        
        // HTTP проверки поверх SSL
        QByteArray request = "HEAD / HTTP/1.1\r\nHost: " + host.toUtf8() + "\r\nConnection: close\r\n\r\n";
        sslSocket->write(request);
        sslSocket->waitForReadyRead(3000);
        
        QByteArray response = sslSocket->readAll();
        QString responseStr = QString::fromUtf8(response);
        
        for (const QString& line : responseStr.split("\r\n")) {
            if (line.toLower().startsWith("server:")) {
                result.httpServer = line.mid(7).trimmed();
            }
        }
        
        result.banner = result.httpServer;
        sslSocket->close();
    }
    sslSocket->deleteLater();
}

void SecurityAuditor::checkSSH(const QString& host, int port, ScanResult& result) {
    // Проверяем SSH
    socket = new QTcpSocket(this);
    socket->connectToHost(host, port);
    
    if (socket->waitForConnected(3000)) {
        socket->waitForReadyRead(3000);
        QByteArray banner = socket->readAll();
        result.sshVersion = QString::fromUtf8(banner).trimmed();
        result.banner = result.sshVersion;
        
        // Проверяем уязвимости SSH
        if (result.sshVersion.contains("SSH-1.")) {
            result.vulnerable = true;
            result.vulnerability = "SSH v1 имеет критические уязвимости!";
        }
        if (result.sshVersion.contains("OpenSSH_7.") || result.sshVersion.contains("OpenSSH_6.")) {
            result.vulnerable = true;
            result.vulnerability = "Устаревшая версия OpenSSH - возможны уязвимости";
        }
        
        socket->close();
    }
    socket->deleteLater();
}

void SecurityAuditor::checkFTP(const QString& host, int port, ScanResult& result) {
    socket = new QTcpSocket(this);
    socket->connectToHost(host, port);
    
    if (socket->waitForConnected(3000)) {
        socket->waitForReadyRead(3000);
        QByteArray banner = socket->readAll();
        result.banner = QString::fromUtf8(banner).trimmed();
        
        // Проверяем анонимный доступ
        socket->write("USER anonymous\r\n");
        socket->waitForReadyRead(2000);
        QByteArray response = socket->readAll();
        
        if (response.contains("331")) {
            socket->write("PASS anonymous\r\n");
            socket->waitForReadyRead(2000);
            QByteArray passResponse = socket->readAll();
            
            if (passResponse.contains("230")) {
                result.ftpAnonymous = "Открыт! Анонимный доступ РАЗРЕШЕН!";
                result.vulnerable = true;
                result.vulnerability = "Анонимный FTP доступ - критическая уязвимость!";
            }
        }
        
        socket->close();
    }
    socket->deleteLater();
}

void SecurityAuditor::checkSMTP(const QString& host, int port, ScanResult& result) {
    socket = new QTcpSocket(this);
    socket->connectToHost(host, port);
    
    if (socket->waitForConnected(3000)) {
        socket->waitForReadyRead(3000);
        result.banner = QString::fromUtf8(socket->readAll()).trimmed();
        
        // Проверяем открытый релей
        socket->write("HELO test\r\n");
        socket->waitForReadyRead(1000);
        socket->readAll();
        
        socket->write("MAIL FROM: <test@test.com>\r\n");
        socket->waitForReadyRead(1000);
        QByteArray mailResponse = socket->readAll();
        
        if (mailResponse.contains("250")) {
            socket->write("RCPT TO: <test@example.com>\r\n");
            socket->waitForReadyRead(1000);
            QByteArray rcptResponse = socket->readAll();
            
            if (rcptResponse.contains("250")) {
                result.vulnerable = true;
                result.vulnerability = "Открытый SMTP релей - может использоваться для спама!";
            }
        }
        
        socket->close();
    }
    socket->deleteLater();
}

void SecurityAuditor::checkMySQL(const QString& host, int port, ScanResult& result) {
    result.banner = grabBanner(host, port);
    
    if (!result.banner.isEmpty()) {
        result.vulnerable = true;
        result.vulnerability = "MySQL доступен извне! Рекомендуется ограничить доступ файрволом.";
    }
}

QString SecurityAuditor::grabBanner(const QString& host, int port) {
    QTcpSocket* tempSocket = new QTcpSocket();
    tempSocket->connectToHost(host, port);
    
    QString banner;
    if (tempSocket->waitForConnected(2000)) {
        tempSocket->waitForReadyRead(2000);
        banner = QString::fromUtf8(tempSocket->readAll()).trimmed();
        tempSocket->close();
    }
    tempSocket->deleteLater();
    return banner;
}

} // namespace SecurityAudit
