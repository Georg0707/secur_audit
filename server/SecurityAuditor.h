#ifndef SECURITYAUDITOR_H
#define SECURITYAUDITOR_H

#include <QObject>
#include <QTcpSocket>
#include <QSslSocket>
#include <QTimer>
#include "PortScanner.h"

namespace SecurityAudit {

class SecurityAuditor : public QObject {
    Q_OBJECT
public:
    explicit SecurityAuditor(QObject *parent = nullptr);
    
    void auditService(const QString& host, int port, ScanResult& result);
    
signals:
    void auditComplete(int port);
    
private slots:
    void onHttpResponse();
    void onSslEncrypted();
    void onBannerReceived();
    void onTimeout();
    
private:
    void checkHTTP(const QString& host, int port, ScanResult& result);
    void checkHTTPS(const QString& host, int port, ScanResult& result);
    void checkSSH(const QString& host, int port, ScanResult& result);
    void checkFTP(const QString& host, int port, ScanResult& result);
    void checkSMTP(const QString& host, int port, ScanResult& result);
    void checkMySQL(const QString& host, int port, ScanResult& result);
    QString grabBanner(const QString& host, int port);
    
    QTcpSocket* socket;
    QSslSocket* sslSocket;
    QTimer* timer;
    QByteArray buffer;
    int currentPort;
    ScanResult* currentResult;
};

} // namespace SecurityAudit

#endif
