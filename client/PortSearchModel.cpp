#include "PortSearchModel.h"
#include <algorithm>

PortSearchModel::PortSearchModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void PortSearchModel::setPortList(const QList<PortInfo>& ports) {
    beginResetModel();
    allPorts = ports;
    filteredPorts = ports;
    endResetModel();
}

void PortSearchModel::filterBySubstring(const QString& substring) {
    beginResetModel();
    
    if (substring.isEmpty()) {
        filteredPorts = allPorts;
    } else {
        filteredPorts.clear();
        QString lowerFilter = substring.toLower();
        
        for (const auto& port : allPorts) {
            // Поиск по номеру порта
            if (QString::number(port.port).contains(substring)) {
                filteredPorts.append(port);
                continue;
            }
            
            // Поиск по названию сервиса
            if (port.service.toLower().contains(lowerFilter)) {
                filteredPorts.append(port);
                continue;
            }
            
            // Поиск по протоколу
            if (port.protocol.toLower().contains(lowerFilter)) {
                filteredPorts.append(port);
                continue;
            }
        }
    }
    
    endResetModel();
}

int PortSearchModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return filteredPorts.size();
}

QVariant PortSearchModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= filteredPorts.size())
        return QVariant();
    
    const PortInfo& port = filteredPorts.at(index.row());
    
    if (role == Qt::DisplayRole) {
        return QString("Порт %1 - %2 (%3)")
            .arg(port.port, 5)
            .arg(port.service)
            .arg(port.protocol);
    } else if (role == Qt::UserRole) {
        return port.port;  // Возвращаем int, а не QString
    } else if (role == Qt::UserRole + 1) {
        return port.service;
    } else if (role == Qt::UserRole + 2) {
        return port.protocol;
    }
    
    return QVariant();
}

PortInfo PortSearchModel::getPortAt(int index) const {
    if (index >= 0 && index < filteredPorts.size()) {
        return filteredPorts.at(index);
    }
    return PortInfo();
}
