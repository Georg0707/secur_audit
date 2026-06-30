#include "SecureClient.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QDataStream>
#include <QSslConfiguration>

SecureClient::SecureClient(QObject *parent)
    : QObject(parent)
{
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    socket.setSslConfiguration(sslConfig);
    
    connect(&socket, &QSslSocket::encrypted, this, &SecureClient::onEncrypted);
    connect(&socket, &QSslSocket::readyRead, this, &SecureClient::onReadyRead);
    connect(&socket, &QSslSocket::disconnected, this, &SecureClient::onDisconnected);
    connect(&socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &SecureClient::onSocketError);
    connect(&socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
            this, &SecureClient::onSslErrors);
}

SecureClient::~SecureClient() {
    disconnectFromServer();
}

void SecureClient::connectToServer(const QString& host, int port) {
    if (socket.isOpen()) {
        socket.close();
    }
    buffer.clear();  // Очищаем буфер при новом подключении
    socket.connectToHostEncrypted(host, port);
}

void SecureClient::disconnectFromServer() {
    if (socket.isOpen()) {
        socket.disconnectFromHost();
    }
}

bool SecureClient::isConnected() const {
    return socket.isOpen() && socket.isEncrypted();
}

void SecureClient::sendScanRequest(const QString& host, const QList<int>& ports) {
    if (!isConnected()) {
        emit errorOccurred("Not connected to server");
        return;
    }
    QByteArray request = Protocol::createScanRequest(host, ports);
    socket.write(request);
    socket.flush();
}

void SecureClient::requestPortList() {
    if (!isConnected()) {
        emit errorOccurred("Not connected to server");
        return;
    }
    QByteArray request = Protocol::createMessage(MessageType::PortListRequest, QJsonObject());
    socket.write(request);
    socket.flush();
}

void SecureClient::onEncrypted() {
    qDebug() << "Connection encrypted successfully";
    emit connected();
    emit connectionStatusChanged(true);
}

void SecureClient::onReadyRead() {
    QByteArray newData = socket.readAll();
    qDebug() << "Received data:" << newData.size() << "bytes";
    buffer.append(newData);
    
    // Обрабатываем все полные сообщения в буфере
    while (buffer.size() >= 4) {
        // Читаем размер сообщения из первых 4 байт
        QDataStream stream(buffer.left(4));
        stream.setByteOrder(QDataStream::BigEndian);
        quint32 size;
        stream >> size;
        
        qDebug() << "Message size:" << size << "buffer size:" << buffer.size();
        
        // Проверяем, что всё сообщение получено
        if (buffer.size() < static_cast<int>(size) + 4) {
            qDebug() << "Waiting for more data...";
            break;
        }
        
        // Извлекаем полное сообщение
        QByteArray messageData = buffer.mid(0, size + 4);
        buffer.remove(0, size + 4);
        
        processMessage(messageData);
    }
}

void SecureClient::onDisconnected() {
    qDebug() << "Disconnected from server";
    emit disconnected();
    emit connectionStatusChanged(false);
}

void SecureClient::onSslErrors(const QList<QSslError>& errors) {
    for (const auto& error : errors) {
        qDebug() << "SSL Error (ignored):" << error.errorString();
    }
    socket.ignoreSslErrors();
}

void SecureClient::onSocketError(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError)
    QString errorMsg = socket.errorString();
    if (!errorMsg.isEmpty() && errorMsg != "Unknown error") {
        emit errorOccurred(errorMsg);
    }
}

void SecureClient::processMessage(const QByteArray& data) {
    auto [type, payload] = Protocol::parseMessage(data);
    
    qDebug() << "Processing message type:" << static_cast<int>(type);
    
    switch (type) {
        case MessageType::Authentication:
            qDebug() << "Authentication message:" << payload["message"].toString();
            break;
            
        case MessageType::PortListResponse: {
            QList<PortInfo> ports;
            QJsonArray portsArray = payload["ports"].toArray();
            for (const auto& portValue : portsArray) {
                ports.append(PortInfo::fromJson(portValue.toObject()));
            }
            qDebug() << "Received port list:" << ports.size() << "ports";
            emit portListReceived(ports);
            break;
        }
            
        case MessageType::ScanResponse: {
            QList<ScanResult> results;
            QJsonArray resultsArray = payload["results"].toArray();
            for (const auto& resultValue : resultsArray) {
                results.append(ScanResult::fromJson(resultValue.toObject()));
            }
            qDebug() << "Received scan results:" << results.size() << "ports";
            emit scanResultReceived(results);
            break;
        }
            
        case MessageType::Error:
            qDebug() << "Error from server:" << payload["error"].toString();
            emit errorOccurred(payload["error"].toString());
            break;
            
        default:
            qDebug() << "Unknown message type:" << static_cast<int>(type);
            break;
    }
}
