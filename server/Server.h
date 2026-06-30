#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QSslSocket>
#include <QSslServer>
#include <QThreadPool>
#include <QList>
#include "../common/Protocol.h"

namespace SecurityAudit {

class PortScanner;

class SecureServer : public QSslServer {
    Q_OBJECT
public:
    explicit SecureServer(QObject *parent = nullptr);
    ~SecureServer();
    
    bool startServer(const QString& configFile);
    void stopServer();
    
protected:
    void incomingConnection(qintptr socketDescriptor) override;
    
private slots:
    void onClientDisconnected();
    void onReadyRead();
    
private:
    void processMessage(QSslSocket* client, const QByteArray& data);
    void sendPortList(QSslSocket* client);
    void handleScanRequest(QSslSocket* client, const QJsonObject& payload);
    void loadConfig(const QString& configFile);
    
    PortScanner* scanner;
    QString configFile;
    int port;
    QString certFile;
    QString keyFile;
    QString portListFile;
    QList<QSslSocket*> clients;
    int maxClients;
    int clientCounter;
};

} // namespace SecurityAudit

#endif
