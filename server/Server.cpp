#include "Server.h"
#include "PortScanner.h"
#include <QFile>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslCertificate>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSettings>
#include <QThread>
#include <iostream>

namespace SecurityAudit {

SecureServer::SecureServer(QObject *parent)
    : QSslServer(parent)
    , scanner(new PortScanner(this))
    , port(8443)
    , maxClients(4)
    , clientCounter(0)
{
}

SecureServer::~SecureServer() {
    stopServer();
}

void SecureServer::loadConfig(const QString& configFile) {
    QSettings settings(configFile, QSettings::IniFormat);
    
    settings.beginGroup("Server");
    port = settings.value("port", 8443).toInt();
    certFile = settings.value("certFile", "../certs/server.crt").toString();
    keyFile = settings.value("keyFile", "../certs/server.key").toString();
    portListFile = settings.value("portList", "../ports").toString();
    maxClients = settings.value("maxClients", 4).toInt();
    settings.endGroup();
    
    std::cout << "Server configuration loaded:" << std::endl;
    std::cout << "  Port: " << port << std::endl;
    std::cout << "  Max clients: " << maxClients << std::endl;
    std::cout << "  Port list: " << portListFile.toStdString() << std::endl;
    std::cout << "  Cert file: " << certFile.toStdString() << std::endl;
    std::cout << "  Key file: " << keyFile.toStdString() << std::endl;
    
    scanner->loadPortList(portListFile);
}

bool SecureServer::startServer(const QString& config) {
    configFile = config;
    loadConfig(configFile);
    
    QSslConfiguration sslConfig;
    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
    
    QFile certFileObj(certFile);
    if (!certFileObj.open(QIODevice::ReadOnly)) {
        std::cerr << "Cannot open certificate file: " << certFile.toStdString() << std::endl;
        return false;
    }
    
    QSslCertificate certificate(&certFileObj, QSsl::Pem);
    certFileObj.close();
    
    if (certificate.isNull()) {
        std::cerr << "Failed to load certificate" << std::endl;
        return false;
    }
    
    QFile keyFileObj(keyFile);
    if (!keyFileObj.open(QIODevice::ReadOnly)) {
        std::cerr << "Cannot open key file: " << keyFile.toStdString() << std::endl;
        return false;
    }
    
    QSslKey sslKey(&keyFileObj, QSsl::Rsa, QSsl::Pem);
    keyFileObj.close();
    
    if (sslKey.isNull()) {
        std::cerr << "Failed to load private key" << std::endl;
        return false;
    }
    
    sslConfig.setLocalCertificate(certificate);
    sslConfig.setPrivateKey(sslKey);
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    
    setSslConfiguration(sslConfig);
    
    if (!listen(QHostAddress::Any, port)) {
        std::cerr << "Cannot start server: " << errorString().toStdString() << std::endl;
        return false;
    }
    
    std::cout << "Secure server started on port " << port << std::endl;
    std::cout << "Waiting for clients..." << std::endl;
    return true;
}

void SecureServer::stopServer() {
    close();
    for (auto client : clients) {
        client->disconnectFromHost();
        client->deleteLater();
    }
    clients.clear();
    std::cout << "Server stopped" << std::endl;
}

void SecureServer::incomingConnection(qintptr socketDescriptor) {
    if (clients.size() >= maxClients) {
        std::cout << "Maximum clients reached, rejecting connection" << std::endl;
        QTcpSocket tempSocket;
        tempSocket.setSocketDescriptor(socketDescriptor);
        tempSocket.disconnectFromHost();
        return;
    }
    
    QSslSocket *sslSocket = new QSslSocket(this);
    sslSocket->setSslConfiguration(sslConfiguration());
    sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
    
    if (sslSocket->setSocketDescriptor(socketDescriptor)) {
        std::cout << "New incoming connection, starting encryption..." << std::endl;
        
        connect(sslSocket, &QSslSocket::encrypted, this, [this, sslSocket]() {
            clientCounter++;
            
            clients.append(sslSocket);
            std::cout << "Client_" << clientCounter << " connected from " 
                      << sslSocket->peerAddress().toString().toStdString() << std::endl;
            std::cout << "Active clients: " << clients.size() << "/" << maxClients << std::endl;
            
            QJsonObject welcomePayload;
            welcomePayload["message"] = "Connected to Security Audit Server";
            welcomePayload["serverVersion"] = "1.0";
            QByteArray welcome = Protocol::createMessage(MessageType::Authentication, welcomePayload);
            sslSocket->write(welcome);
            
            sendPortList(sslSocket);
        });
        
        connect(sslSocket, &QSslSocket::disconnected, this, &SecureServer::onClientDisconnected);
        connect(sslSocket, &QSslSocket::readyRead, this, &SecureServer::onReadyRead);
        
        connect(sslSocket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
                this, [sslSocket](const QList<QSslError>& errors) {
            for (const auto& error : errors) {
                std::cerr << "SSL Error: " << error.errorString().toStdString() << std::endl;
            }
            sslSocket->ignoreSslErrors();
        });
        
        sslSocket->startServerEncryption();
    } else {
        std::cerr << "Failed to set socket descriptor" << std::endl;
        delete sslSocket;
    }
}

void SecureServer::onClientDisconnected() {
    QSslSocket *client = qobject_cast<QSslSocket*>(sender());
    if (client) {
        std::cout << "Client disconnected" << std::endl;
        clients.removeAll(client);
        client->deleteLater();
        std::cout << "Active clients: " << clients.size() << "/" << maxClients << std::endl;
    }
}

void SecureServer::onReadyRead() {
    QSslSocket *client = qobject_cast<QSslSocket*>(sender());
    if (!client) return;
    
    QByteArray data = client->readAll();
    processMessage(client, data);
}

void SecureServer::sendPortList(QSslSocket* client) {
    QList<PortInfo> portList = scanner->getPortList();
    QByteArray response = Protocol::createPortListResponse(portList);
    client->write(response);
    std::cout << "Sent port list (" << portList.size() << " ports) to client" << std::endl;
}

void SecureServer::handleScanRequest(QSslSocket* client, const QJsonObject& payload) {
    QString host = payload["host"].toString();
    QJsonArray portsArray = payload["ports"].toArray();
    
    QList<int> ports;
    for (const auto& portValue : portsArray) {
        ports.append(portValue.toInt());
    }
    
    std::cout << "Scan request for host " << host.toStdString() 
              << " (" << ports.size() << " ports)" << std::endl;
    
    // Создаем поток и сканер
    QThread* thread = new QThread();
    PortScanner* threadScanner = new PortScanner();
    threadScanner->loadPortList(portListFile);
    threadScanner->moveToThread(thread);
    
    // Запускаем сканирование при старте потока
    connect(thread, &QThread::started, threadScanner, [threadScanner, host, ports]() {
        threadScanner->scanHost(host, ports);
    });
    
    // По завершении сканирования отправляем результат и останавливаем поток
    connect(threadScanner, &PortScanner::scanComplete, this,
            [this, client, threadScanner, thread](const QList<ScanResult>& results) {
        QByteArray response = Protocol::createScanResponse(results);
        if (client && client->isOpen()) {
            client->write(response);
            std::cout << "Scan complete, sent results to client" << std::endl;
        }
        
        // Останавливаем поток
        thread->quit();
        thread->wait(5000);  // Ждем до 5 секунд завершения потока
        
        // Удаляем объекты
        threadScanner->deleteLater();
        thread->deleteLater();
    });
    
    // Обработка ошибок
    connect(threadScanner, &PortScanner::error, this,
            [this, client](const QString& error) {
        if (client && client->isOpen()) {
            QJsonObject errorPayload;
            errorPayload["error"] = error;
            QByteArray errorMsg = Protocol::createMessage(MessageType::Error, errorPayload);
            client->write(errorMsg);
        }
    });
    
    // Запускаем поток
    thread->start();
}

void SecureServer::processMessage(QSslSocket* client, const QByteArray& data) {
    auto [type, payload] = Protocol::parseMessage(data);
    
    switch (type) {
        case MessageType::ScanRequest:
            handleScanRequest(client, payload);
            break;
            
        case MessageType::PortListRequest:
            sendPortList(client);
            break;
            
        case MessageType::Disconnect:
            client->disconnectFromHost();
            break;
            
        default:
            QJsonObject errorPayload;
            errorPayload["error"] = "Unknown message type";
            QByteArray errorMsg = Protocol::createMessage(MessageType::Error, errorPayload);
            client->write(errorMsg);
            break;
    }
}

} // namespace SecurityAudit
