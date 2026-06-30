#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QList>

namespace SecurityAudit {

enum class MessageType {
    ScanRequest = 1,
    ScanResponse = 2,
    PortListRequest = 3,
    PortListResponse = 4,
    Error = 5,
    Authentication = 6,
    Disconnect = 7
};

struct PortInfo {
    int port;
    QString service;
    QString protocol;
    
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["port"] = port;
        obj["service"] = service;
        obj["protocol"] = protocol;
        return obj;
    }
    
    static PortInfo fromJson(const QJsonObject& obj) {
        PortInfo info;
        info.port = obj["port"].toInt();
        info.service = obj["service"].toString();
        info.protocol = obj["protocol"].toString();
        return info;
    }
};

struct ScanResult {
    int port;
    QString service;
    QString protocol;
    bool isOpen;
    QString error;
    qint64 responseTime;
    
    // Дополнительные поля аудита безопасности
    QString banner;              // Баннер сервиса
    QString sslVersion;          // Версия SSL/TLS
    QString sslCipher;           // Шифр SSL
    bool sslExpired;            // Просрочен ли сертификат
    QString httpServer;          // Заголовок Server
    QStringList httpMethods;     // Поддерживаемые HTTP методы
    bool hasHttpsRedirect;      // Редирект с HTTP на HTTPS
    QStringList securityHeaders; // Заголовки безопасности
    QString sshVersion;          // Версия SSH
    QString ftpAnonymous;        // Доступен ли анонимный FTP
    bool vulnerable;             // Обнаружена ли уязвимость
    QString vulnerability;       // Описание уязвимости
    
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["port"] = port;
        obj["service"] = service;
        obj["protocol"] = protocol;
        obj["isOpen"] = isOpen;
        obj["error"] = error;
        obj["responseTime"] = responseTime;
        obj["banner"] = banner;
        obj["sslVersion"] = sslVersion;
        obj["sslCipher"] = sslCipher;
        obj["sslExpired"] = sslExpired;
        obj["httpServer"] = httpServer;
        obj["hasHttpsRedirect"] = hasHttpsRedirect;
        obj["sshVersion"] = sshVersion;
        obj["ftpAnonymous"] = ftpAnonymous;
        obj["vulnerable"] = vulnerable;
        obj["vulnerability"] = vulnerability;
        
        QJsonArray methods;
        for (const auto& m : httpMethods) methods.append(m);
        obj["httpMethods"] = methods;
        
        QJsonArray headers;
        for (const auto& h : securityHeaders) headers.append(h);
        obj["securityHeaders"] = headers;
        
        return obj;
    }
    
    static ScanResult fromJson(const QJsonObject& obj) {
        ScanResult result;
        result.port = obj["port"].toInt();
        result.service = obj["service"].toString();
        result.protocol = obj["protocol"].toString();
        result.isOpen = obj["isOpen"].toBool();
        result.error = obj["error"].toString();
        result.responseTime = obj["responseTime"].toVariant().toLongLong();
        result.banner = obj["banner"].toString();
        result.sslVersion = obj["sslVersion"].toString();
        result.sslCipher = obj["sslCipher"].toString();
        result.sslExpired = obj["sslExpired"].toBool();
        result.httpServer = obj["httpServer"].toString();
        result.hasHttpsRedirect = obj["hasHttpsRedirect"].toBool();
        result.sshVersion = obj["sshVersion"].toString();
        result.ftpAnonymous = obj["ftpAnonymous"].toString();
        result.vulnerable = obj["vulnerable"].toBool();
        result.vulnerability = obj["vulnerability"].toString();
        
        QJsonArray methods = obj["httpMethods"].toArray();
        for (const auto& m : methods) result.httpMethods.append(m.toString());
        
        QJsonArray headers = obj["securityHeaders"].toArray();
        for (const auto& h : headers) result.securityHeaders.append(h.toString());
        
        return result;
    }
};

class Protocol {
public:
    static QByteArray createMessage(MessageType type, const QJsonObject& payload);
    static QPair<MessageType, QJsonObject> parseMessage(const QByteArray& data);
    static QByteArray createScanRequest(const QString& host, const QList<int>& ports);
    static QByteArray createPortListResponse(const QList<PortInfo>& ports);
    static QByteArray createScanResponse(const QList<ScanResult>& results);
};

} // namespace SecurityAudit

#endif
