#ifndef PORTSEARCHMODEL_H
#define PORTSEARCHMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "../common/Protocol.h"

using namespace SecurityAudit;

class PortSearchModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit PortSearchModel(QObject *parent = nullptr);
    
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    
    void setPortList(const QList<PortInfo>& ports);
    void filterBySubstring(const QString& substring); // Посимвольная фильтрация
    
    PortInfo getPortAt(int index) const;
    
private:
    QList<PortInfo> allPorts;
    QList<PortInfo> filteredPorts;
};

#endif
