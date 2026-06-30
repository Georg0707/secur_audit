#ifndef SECURECLIENT_H
#define SECURECLIENT_H

#include <QObject>
#include <QSslSocket>
#include <QTimer>
#include "../common/Protocol.h"

using namespace SecurityAudit;

class SecureClient : public QObject {
    Q_OBJECT
public:
    explicit SecureClient(QObject *parent = nullptr);
    ~SecureClient();
    
    void connectToServer(const QString& host, int port);
    void disconnectFromServer();
    void sendScanRequest(const QString& host, const QList<int>& ports);
    void requestPortList();
    bool isConnected() const;
    
signals:
    void connected();
    void disconnected();
    void portListReceived(const QList<PortInfo>& ports);
    void scanResultReceived(const QList<ScanResult>& results);
    void errorOccurred(const QString& message);
    void connectionStatusChanged(bool connected);
    
private slots:
    void onEncrypted();
    void onReadyRead();
    void onDisconnected();
    void onSslErrors(const QList<QSslError>& errors);
    void onSocketError(QAbstractSocket::SocketError socketError);
    
private:
    void processMessage(const QByteArray& data);
    
    QSslSocket socket;
    QByteArray buffer;
};

#endif
