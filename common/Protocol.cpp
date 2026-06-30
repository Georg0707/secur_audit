#include "Protocol.h"
#include <QDataStream>
#include <QIODevice>

namespace SecurityAudit {

QByteArray Protocol::createMessage(MessageType type, const QJsonObject& payload) {
    QJsonObject message;
    message["type"] = static_cast<int>(type);
    message["payload"] = payload;
    message["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QByteArray data = QJsonDocument(message).toJson(QJsonDocument::Compact);
    
    // Добавляем заголовок с размером сообщения
    QByteArray header;
    QDataStream stream(&header, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << static_cast<quint32>(data.size());
    
    return header + data;
}

QPair<MessageType, QJsonObject> Protocol::parseMessage(const QByteArray& data) {
    if (data.size() < 4) {
        return {MessageType::Error, {{"error", "Invalid message size"}}};
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    quint32 size;
    stream >> size;
    
    if (data.size() < static_cast<int>(size) + 4) {
        return {MessageType::Error, {{"error", "Incomplete message"}}};
    }
    
    QByteArray jsonData = data.mid(4, size);
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON parse error:" << parseError.errorString();
        qDebug() << "Data received:" << jsonData;
        return {MessageType::Error, {{"error", "Invalid JSON: " + parseError.errorString()}}};
    }
    
    if (!doc.isObject()) {
        return {MessageType::Error, {{"error", "JSON is not an object"}}};
    }
    
    QJsonObject obj = doc.object();
    MessageType type = static_cast<MessageType>(obj["type"].toInt());
    QJsonObject payload = obj["payload"].toObject();
    
    return {type, payload};
}

QByteArray Protocol::createScanRequest(const QString& host, const QList<int>& ports) {
    QJsonObject payload;
    payload["host"] = host;
    QJsonArray portsArray;
    for (int port : ports) {
        portsArray.append(port);
    }
    payload["ports"] = portsArray;
    return createMessage(MessageType::ScanRequest, payload);
}

QByteArray Protocol::createPortListResponse(const QList<PortInfo>& ports) {
    QJsonObject payload;
    QJsonArray portsArray;
    for (const auto& port : ports) {
        portsArray.append(port.toJson());
    }
    payload["ports"] = portsArray;
    payload["total"] = ports.size();
    return createMessage(MessageType::PortListResponse, payload);
}

QByteArray Protocol::createScanResponse(const QList<ScanResult>& results) {
    QJsonObject payload;
    QJsonArray resultsArray;
    for (const auto& result : results) {
        resultsArray.append(result.toJson());
    }
    payload["results"] = resultsArray;
    payload["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    return createMessage(MessageType::ScanResponse, payload);
}

} // namespace SecurityAudit
