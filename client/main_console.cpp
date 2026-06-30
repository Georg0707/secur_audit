#include <QCoreApplication>
#include <QTextStream>
#include <QThread>
#include <iostream>
#include "SecureClient.h"

using namespace SecurityAudit;

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    QTextStream cin(stdin);
    QTextStream cout(stdout);
    
    SecureClient client;
    QList<PortInfo> portList;
    
    cout << "\n=== Security Audit Client (Console Mode) ===" << endl;
    cout << "Connecting to server..." << endl;
    
    QObject::connect(&client, &SecureClient::connected, [&]() {
        cout << "Connected to server (SSL secured)" << endl;
        cout << "Requesting port list..." << endl;
        client.requestPortList();
    });
    
    QObject::connect(&client, &SecureClient::disconnected, [&]() {
        cout << "Disconnected from server" << endl;
    });
    
    QObject::connect(&client, &SecureClient::portListReceived, [&](const QList<PortInfo>& ports) {
        portList = ports;
        cout << "\nReceived " << ports.size() << " ports from server" << endl;
        cout << "\nAvailable commands:" << endl;
        cout << "  scan <host> [filter]  - scan host with optional port filter" << endl;
        cout << "  ports [filter]        - show port list with optional filter" << endl;
        cout << "  help                  - show this help" << endl;
        cout << "  quit                  - exit program" << endl;
        cout << "\nExamples:" << endl;
        cout << "  scan google.com             - scan all 1000 ports" << endl;
        cout << "  scan google.com http        - scan only HTTP ports" << endl;
        cout << "  scan localhost 22 80 443    - scan specific ports" << endl;
        cout << "  ports ssh                   - show SSH-related ports" << endl;
    });
    
    QObject::connect(&client, &SecureClient::scanResultReceived, [&](const QList<ScanResult>& results) {
        cout << "\n=== SCAN RESULTS ===" << endl;
        
        int openCount = 0;
        for (const auto& res : results) {
            if (res.isOpen) {
                cout << "  [OPEN]   ";
                openCount++;
            } else {
                cout << "  [CLOSED] ";
            }
            cout << QString("Port %1 (%2) - %3")
                .arg(res.port, 5)
                .arg(res.protocol)
                .arg(res.service)
                .toStdString() << endl;
        }
        
        cout << "----------------------------------------" << endl;
        cout << "Total: " << openCount << " open, " 
             << (results.size() - openCount) << " closed out of " 
             << results.size() << " scanned" << endl;
        cout << "========================================\n" << endl;
    });
    
    QObject::connect(&client, &SecureClient::errorOccurred, [&](const QString& msg) {
        cout << "ERROR: " << msg.toStdString() << endl;
    });
    
    client.connectToServer("localhost", 8443);
    
    while (true) {
        cout << "> " << flush;
        QString line = cin.readLine();
        
        if (line.isEmpty()) continue;
        if (line == "quit" || line == "exit") break;
        
        if (line == "help") {
            cout << "\nCommands:" << endl;
            cout << "  scan <host> [filter|port1 port2 ...]" << endl;
            cout << "       if filter is text - scan ports matching filter" << endl;
            cout << "       if filter is numbers - scan specific ports" << endl;
            cout << "       if no filter - scan ALL 1000 ports" << endl;
            cout << "  ports [filter]" << endl;
            cout << "  quit" << endl << endl;
        }
        else if (line.startsWith("ports")) {
            QString filter = line.mid(5).trimmed();
            cout << "\nPort list";
            if (!filter.isEmpty()) cout << " (filter: " << filter << ")";
            cout << ":" << endl;
            
            int shown = 0;
            for (const auto& port : portList) {
                if (!filter.isEmpty()) {
                    if (!port.service.contains(filter, Qt::CaseInsensitive) &&
                        !QString::number(port.port).contains(filter) &&
                        !port.protocol.contains(filter, Qt::CaseInsensitive)) {
                        continue;
                    }
                }
                
                cout << QString("  %1. Port %2 - %3 (%4)")
                    .arg(++shown, 3)
                    .arg(port.port, 5)
                    .arg(port.service, -25)
                    .arg(port.protocol)
                    .toStdString() << endl;
                    
                if (shown >= 50) {
                    cout << "  ... showing first 50 matches" << endl;
                    break;
                }
            }
            cout << "  Total: " << shown << " ports shown" << endl << endl;
        }
        else if (line.startsWith("scan ")) {
            QStringList parts = line.mid(5).split(" ", Qt::SkipEmptyParts);
            if (parts.size() < 1) {
                cout << "Usage: scan <host> [filter|ports...]" << endl;
                continue;
            }
            
            QString host = parts[0];
            QList<int> portsToScan;
            
            if (parts.size() == 1) {
                // Сканируем ВСЕ порты из базы
                cout << "\nPreparing to scan ALL " << portList.size() << " ports..." << endl;
                for (const auto& port : portList) {
                    portsToScan.append(port.port);
                }
            }
            else {
                // Проверяем, являются ли остальные аргументы числами
                bool allNumbers = true;
                for (int i = 1; i < parts.size(); i++) {
                    bool ok;
                    parts[i].toInt(&ok);
                    if (!ok) {
                        allNumbers = false;
                        break;
                    }
                }
                
                if (allNumbers) {
                    // Сканируем указанные порты
                    for (int i = 1; i < parts.size(); i++) {
                        int port = parts[i].toInt();
                        if (port > 0 && port < 65536) {
                            portsToScan.append(port);
                        }
                    }
                } else {
                    // Используем остаток как текстовый фильтр
                    QString filter = QStringList(parts.mid(1)).join(" ").toLower();
                    for (const auto& port : portList) {
                        if (port.service.toLower().contains(filter) ||
                            QString::number(port.port).contains(filter) ||
                            port.protocol.toLower().contains(filter)) {
                            portsToScan.append(port.port);
                        }
                    }
                }
            }
            
            if (!portsToScan.isEmpty()) {
                cout << "Scanning " << host.toStdString() << " on " 
                     << portsToScan.size() << " ports..." << endl;
                client.sendScanRequest(host, portsToScan);
            } else {
                cout << "No ports to scan." << endl;
            }
        }
        else {
            cout << "Unknown command. Type 'help' for available commands." << endl;
        }
    }
    
    cout << "\nDisconnecting..." << endl;
    client.disconnectFromServer();
    QThread::msleep(100);
    
    return 0;
}
