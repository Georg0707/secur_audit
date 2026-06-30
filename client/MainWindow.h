#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QListWidget>
#include "SecureClient.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onHostTextChanged(const QString& text);
    void onSuggestionClicked(QListWidgetItem* item);
    void onScanClicked();
    void onPortListReceived(const QList<PortInfo>& ports);
    void onScanResultReceived(const QList<ScanResult>& results);
    void onError(const QString& message);
    
private:
    void setupUI();
    void updateSuggestions(const QString& text);
    
    QLineEdit* hostInput;           // Поле ввода сайта
    QListWidget* suggestionsList;   // Выпадающие подсказки
    QPushButton* scanButton;        // Кнопка проверки
    QTextEdit* resultsView;         // Результаты
    QLabel* statusLabel;            // Статус
    
    SecureClient client;
    QList<PortInfo> allPorts;
    QStringList hostHistory;        // История введенных сайтов
    bool isConnected;
};

#endif
