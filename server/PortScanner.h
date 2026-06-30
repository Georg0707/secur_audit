#ifndef PORTSCANNER_H
#define PORTSCANNER_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QMutex>
#include "../common/Protocol.h"

namespace SecurityAudit {

class PortScanner : public QObject {
    Q_OBJECT
public:
    explicit PortScanner(QObject *parent = nullptr);
    ~PortScanner();
    
    void scanHost(const QString& host, const QList<int>& ports, int timeout = 3000);
    void loadPortList(const QString& filename);
    QList<PortInfo> getPortList() const;
    
signals:
    void scanProgress(int port, bool isOpen);
    void scanComplete(const QList<ScanResult>& results);
    void error(const QString& message);
    
private slots:
    void onConnected();
    void onError(QAbstractSocket::SocketError error);
    void onTimeout();
    void startNextScan();  // Новый слот для запуска из event loop потока
    
private:
    void scanNextPort();
    void createSocketAndConnect();
    
    bool isScanning;
    QString currentHost;
    QList<int> portsToScan;
    int currentPortIndex;
    int scanTimeout;
    QTcpSocket* socket;
    QTimer* timer;
    QList<ScanResult> results;
    mutable QMutex mutex;
    QList<PortInfo> portDatabase;
};

} // namespace SecurityAudit

#endif
