#include "PortScanner.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QThread>

namespace SecurityAudit {

PortScanner::PortScanner(QObject *parent) 
    : QObject(parent)
    , isScanning(false)
    , currentPortIndex(0)
    , scanTimeout(3000)
    , socket(nullptr)
    , timer(nullptr)
{
}

PortScanner::~PortScanner() {
    if (socket) {
        socket->disconnect();
        socket->deleteLater();
        socket = nullptr;
    }
    if (timer) {
        timer->stop();
        timer->deleteLater();
        timer = nullptr;
    }
}

void PortScanner::loadPortList(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit error("Cannot open port list file: " + filename);
        return;
    }
    
    QTextStream in(&file);
    portDatabase.clear();
   
    QRegularExpression regex("^(\\d+)\\.\\s+([^-]+)\\s*-\\s*(.+)$");
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        
        QRegularExpressionMatch match = regex.match(line);
        if (match.hasMatch()) {
            PortInfo info;
            info.port = match.captured(1).toInt();
            info.service = match.captured(2).trimmed();
            info.protocol = "TCP";
           
            QString description = match.captured(3).toLower();
            if (description.contains("udp")) {
                info.protocol = "TCP/UDP";
            }
            
            portDatabase.append(info);
        }
    }
    
    file.close();
    qDebug() << "Loaded" << portDatabase.size() << "ports from database";
   
    for (int i = 0; i < qMin(10, portDatabase.size()); i++) {
        qDebug() << "  Port:" << portDatabase[i].port 
                 << "Service:" << portDatabase[i].service 
                 << "Protocol:" << portDatabase[i].protocol;
    }
}

QList<PortInfo> PortScanner::getPortList() const {
    QMutexLocker locker(&mutex);
    return portDatabase;
}

void PortScanner::scanHost(const QString& host, const QList<int>& ports, int timeout) {
    if (isScanning) {
        emit error("Scan already in progress");
        return;
    }
    
    currentHost = host;
    portsToScan = ports;
    currentPortIndex = 0;
    scanTimeout = timeout;
    results.clear();
    isScanning = true;
    
    qDebug() << "Starting scan of" << host << "with" << ports.size() << "ports in thread" << QThread::currentThreadId();
    
    QMetaObject::invokeMethod(this, "startNextScan", Qt::QueuedConnection);
}

void PortScanner::startNextScan() {
    if (!isScanning) return;
    scanNextPort();
}

void PortScanner::createSocketAndConnect() {
    if (socket) {
        socket->deleteLater();
    }
    
    socket = new QTcpSocket(this);
    socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    
    connect(socket, &QTcpSocket::connected, this, &PortScanner::onConnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &PortScanner::onError);
    
    if (timer) {
        timer->stop();
        timer->deleteLater();
    }
    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &PortScanner::onTimeout);
    
    int port = portsToScan[currentPortIndex];
    socket->connectToHost(currentHost, port);
    timer->start(scanTimeout);
}

void PortScanner::scanNextPort() {
    if (currentPortIndex >= portsToScan.size()) {
        isScanning = false;
        qDebug() << "Scan complete, emitting results";
        emit scanComplete(results);
        return;
    }
    
    createSocketAndConnect();
}

void PortScanner::onConnected() {
    if (timer) timer->stop();
    
    int port = portsToScan[currentPortIndex];
    ScanResult result;
    result.port = port;
    result.protocol = "TCP";
    
    for (const auto& info : portDatabase) {
        if (info.port == port) {
            result.service = info.service;
            result.protocol = info.protocol;
            break;
        }
    }
    
    if (result.service.isEmpty()) {
        result.service = "unknown";
    }
    
    result.isOpen = true;
    result.responseTime = 0;
    results.append(result);
    
    qDebug() << "Port" << port << "is OPEN -" << result.service;
    emit scanProgress(port, true);
    
    if (socket) {
        socket->disconnect();
        socket->deleteLater();
        socket = nullptr;
    }
    
    currentPortIndex++;
    scanNextPort();
}

void PortScanner::onError(QAbstractSocket::SocketError error) {
    Q_UNUSED(error)
    if (timer) timer->stop();
    
    int port = portsToScan[currentPortIndex];
    ScanResult result;
    result.port = port;
    result.protocol = "TCP";
    
    for (const auto& info : portDatabase) {
        if (info.port == port) {
            result.service = info.service;
            result.protocol = info.protocol;
            break;
        }
    }
    
    if (result.service.isEmpty()) {
        result.service = "unknown";
    }
    
    result.isOpen = false;
    if (socket) {
        result.error = socket->errorString();
    }
    results.append(result);
    
    if (socket) {
        socket->disconnect();
        socket->deleteLater();
        socket = nullptr;
    }
    
    currentPortIndex++;
    scanNextPort();
}

void PortScanner::onTimeout() {
    if (timer) timer->stop();
    
    int port = portsToScan[currentPortIndex];
    ScanResult result;
    result.port = port;
    result.protocol = "TCP";
    
    for (const auto& info : portDatabase) {
        if (info.port == port) {
            result.service = info.service;
            result.protocol = info.protocol;
            break;
        }
    }
    
    if (result.service.isEmpty()) {
        result.service = "unknown";
    }
    
    result.isOpen = false;
    result.error = "Timeout";
    results.append(result);
    
    if (socket) {
        socket->abort();
        socket->deleteLater();
        socket = nullptr;
    }
    
    currentPortIndex++;
    scanNextPort();
}

}
