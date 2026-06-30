#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QMessageBox>
#include <QTimer>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , isConnected(false)
{
    setupUI();
    
    connect(&client, &SecureClient::connected, this, [this]() {
        statusLabel->setText("✓ Подключено к серверу");
        statusLabel->setStyleSheet("color: #4CAF50; font-weight: bold; padding: 5px;");
        scanButton->setEnabled(true);
        hostInput->setEnabled(true);
        isConnected = true;
    });
    
    connect(&client, &SecureClient::disconnected, this, [this]() {
        statusLabel->setText("✗ Нет подключения");
        statusLabel->setStyleSheet("color: #f44336; font-weight: bold; padding: 5px;");
        scanButton->setEnabled(false);
        hostInput->setEnabled(false);
        suggestionsList->hide();
        isConnected = false;
    });
    
    connect(&client, &SecureClient::portListReceived, this, &MainWindow::onPortListReceived);
    connect(&client, &SecureClient::scanResultReceived, this, &MainWindow::onScanResultReceived);
    connect(&client, &SecureClient::errorOccurred, this, &MainWindow::onError);
    
    setWindowTitle("Аудит безопасности сайта");
    setFixedSize(600, 550);
    
    QTimer::singleShot(500, this, [this]() {
        client.connectToServer("localhost", 8443);
    });
}

MainWindow::~MainWindow() {
    if (client.isConnected()) {
        client.disconnectFromServer();
    }
}

void MainWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color: #ffffff;");
    setCentralWidget(centralWidget);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // Заголовок
    QLabel* titleLabel = new QLabel("Аудит безопасности сайта");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #333;");
    mainLayout->addWidget(titleLabel);
    
    statusLabel = new QLabel("Подключение к серверу...");
    statusLabel->setStyleSheet("color: #FF9800; font-weight: bold;");
    mainLayout->addWidget(statusLabel);
    
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #e0e0e0;");
    mainLayout->addWidget(line);
    
    // Поле ввода
    QLabel* inputLabel = new QLabel("Введите адрес сайта:");
    inputLabel->setStyleSheet("font-weight: bold; color: #333;");
    mainLayout->addWidget(inputLabel);
    
    hostInput = new QLineEdit();
    hostInput->setPlaceholderText("например: google.com");
    hostInput->setEnabled(false);
    hostInput->setMinimumHeight(38);
    hostInput->setStyleSheet("font-size: 15px; padding: 8px; border: 2px solid #e0e0e0; border-radius: 6px;");
    mainLayout->addWidget(hostInput);
    
    suggestionsList = new QListWidget();
    suggestionsList->setMaximumHeight(150);
    suggestionsList->setStyleSheet("border: 2px solid #2196F3; border-radius: 4px; font-size: 13px;");
    suggestionsList->hide();
    mainLayout->addWidget(suggestionsList);
    
    // Кнопка
    scanButton = new QPushButton("Провести аудит безопасности");
    scanButton->setEnabled(false);
    scanButton->setMinimumHeight(42);
    scanButton->setCursor(Qt::PointingHandCursor);
    scanButton->setStyleSheet(
        "QPushButton { background-color: #f44336; color: white; font-size: 14px; font-weight: bold; border-radius: 6px; }"
        "QPushButton:hover { background-color: #d32f2f; }"
        "QPushButton:disabled { background-color: #BDBDBD; }"
    );
    mainLayout->addWidget(scanButton);
    
    // Результаты
    resultsView = new QTextEdit();
    resultsView->setReadOnly(true);
    resultsView->setPlaceholderText("Результаты аудита безопасности...");
    resultsView->setMinimumHeight(250);
    resultsView->setStyleSheet("font-family: 'Courier New', monospace; font-size: 11px; border: 2px solid #e0e0e0; border-radius: 6px; padding: 8px;");
    mainLayout->addWidget(resultsView);
    
    connect(hostInput, &QLineEdit::textChanged, this, &MainWindow::onHostTextChanged);
    connect(suggestionsList, &QListWidget::itemClicked, this, &MainWindow::onSuggestionClicked);
    connect(scanButton, &QPushButton::clicked, this, &MainWindow::onScanClicked);
    
    hostHistory << "google.com" << "youtube.com" << "github.com" 
                << "localhost" << "stackoverflow.com" << "wikipedia.org"
                << "reddit.com" << "amazon.com" << "facebook.com"
                << "twitter.com" << "microsoft.com" << "apple.com"
                << "yahoo.com" << "linkedin.com" << "ebay.com"
                << "paypal.com" << "spotify.com" << "gitlab.com";
}

void MainWindow::onHostTextChanged(const QString& text) {
    if (text.isEmpty()) { suggestionsList->hide(); return; }
    
    suggestionsList->clear();
    QString lowerText = text.toLower().trimmed();
    QStringList matches;
    
    for (const QString& host : hostHistory) {
        if (host.startsWith(lowerText)) matches.prepend(host);
        else if (host.contains(lowerText)) matches.append(host);
    }
    
    QStringList unique;
    for (const QString& m : matches) {
        if (!unique.contains(m)) unique.append(m);
    }
    
    if (text.contains(".") && !unique.contains(text)) unique.prepend(text);
    
    if (unique.isEmpty()) {
        suggestionsList->addItem("Например: " + text + ".com");
    } else {
        for (int i = 0; i < qMin(10, unique.size()); i++) {
            QListWidgetItem* item = new QListWidgetItem(unique[i]);
            if (i == 0) { QFont f = item->font(); f.setBold(true); item->setFont(f); }
            suggestionsList->addItem(item);
        }
    }
    suggestionsList->show();
    suggestionsList->setCurrentRow(0);
}

void MainWindow::onSuggestionClicked(QListWidgetItem* item) {
    QString text = item->text();
    if (!text.startsWith("Например")) {
        hostInput->setText(text);
    }
    suggestionsList->hide();
    hostInput->setFocus();
}

void MainWindow::onScanClicked() {
    QString host = hostInput->text().trimmed();
    if (host.isEmpty()) { QMessageBox::warning(this, "Ошибка", "Введите адрес сайта"); return; }
    if (host.contains(":")) host = host.left(host.indexOf(":"));
    
    // Проверяем основные порты для аудита безопасности
    QList<int> portsToScan;
    portsToScan << 80 << 443 << 8080 << 8443  // HTTP/HTTPS
                << 21 << 22 << 25 << 110 << 143 << 993 << 995  // FTP, SSH, почта
                << 3306 << 5432 << 27017 << 6379;  // Базы данных
    
    resultsView->clear();
    resultsView->append("╔══════════════════════════════════════╗");
    resultsView->append("║   АУДИТ БЕЗОПАСНОСТИ САЙТА          ║");
    resultsView->append("╚══════════════════════════════════════╝");
    resultsView->append("");
    resultsView->append("Цель: " + host);
    resultsView->append("Проверка " + QString::number(portsToScan.size()) + " портов на наличие уязвимостей...\n");
    
    scanButton->setEnabled(false);
    scanButton->setText("Аудит...");
    hostInput->setEnabled(false);
    suggestionsList->hide();
    
    client.sendScanRequest(host, portsToScan);
    
    if (!hostHistory.contains(host)) {
        hostHistory.prepend(host);
        if (hostHistory.size() > 50) hostHistory.removeLast();
    }
}

void MainWindow::onPortListReceived(const QList<PortInfo>& ports) {
    allPorts = ports;
}

void MainWindow::onScanResultReceived(const QList<ScanResult>& results) {
    int openCount = 0;
    int vulnCount = 0;
    QList<ScanResult> openPorts;
    
    for (const auto& res : results) {
        if (res.isOpen) {
            openPorts.append(res);
            openCount++;
            if (res.vulnerable) vulnCount++;
        }
    }
    
    std::sort(openPorts.begin(), openPorts.end(),
              [](const ScanResult& a, const ScanResult& b) { return a.port < b.port; });
    
    resultsView->clear();
    resultsView->append("╔══════════════════════════════════════╗");
    resultsView->append("║        РЕЗУЛЬТАТЫ АУДИТА            ║");
    resultsView->append("╚══════════════════════════════════════╝\n");
    
    // Сводка
    resultsView->append("┌─ СВОДКА ────────────────────────────┐");
    resultsView->append(QString("│ Открытых портов: %1").arg(openCount, 2));
    resultsView->append(QString("│ Найдено уязвимостей: %1").arg(vulnCount, 2));
    if (vulnCount > 0) {
        resultsView->append("│ ⚠ ТРЕБУЕТСЯ ВНИМАНИЕ!              │");
    } else if (openCount > 0) {
        resultsView->append("│ ✓ Критических проблем не найдено    │");
    }
    resultsView->append("└──────────────────────────────────────┘\n");
    
    // Детали по каждому порту
    for (const auto& res : openPorts) {
        // Заголовок порта
        QString header = QString("ПОРТ %1 (%2)").arg(res.port).arg(res.service);
        resultsView->append("┌─ " + header + " " + QString("─").repeated(qMax(1, 35 - header.length())) + "┐");
        
        // Тип сервиса
        QString serviceType;
        switch(res.port) {
            case 80: serviceType = "HTTP (веб-сервер)"; break;
            case 443: serviceType = "HTTPS (защищенный веб-сервер)"; break;
            case 8080: serviceType = "HTTP (альтернативный)"; break;
            case 8443: serviceType = "HTTPS (альтернативный)"; break;
            case 22: serviceType = "SSH (удаленное управление)"; break;
            case 21: serviceType = "FTP (файловый сервер)"; break;
            case 25: serviceType = "SMTP (почтовый сервер)"; break;
            case 110: serviceType = "POP3 (почтовый протокол)"; break;
            case 143: serviceType = "IMAP (почтовый протокол)"; break;
            case 993: serviceType = "IMAPS (защищенная почта)"; break;
            case 995: serviceType = "POP3S (защищенная почта)"; break;
            case 3306: serviceType = "MySQL (база данных)"; break;
            case 5432: serviceType = "PostgreSQL (база данных)"; break;
            case 27017: serviceType = "MongoDB (база данных)"; break;
            case 6379: serviceType = "Redis (кэш/БД)"; break;
            default: serviceType = "Сетевой сервис";
        }
        resultsView->append("│ Тип: " + serviceType);
        
        // SSL информация
        if (!res.sslVersion.isEmpty()) {
            resultsView->append("│ SSL/TLS: " + res.sslVersion + " (" + res.sslCipher + ")");
            if (res.sslExpired) {
                resultsView->append("│ ⚠ Сертификат ПРОСРОЧЕН!");
            }
        }
        
        // HTTP сервер
        if (!res.httpServer.isEmpty()) {
            resultsView->append("│ Сервер: " + res.httpServer);
        }
        
        // Заголовки безопасности
        if (!res.securityHeaders.isEmpty()) {
            resultsView->append("│ Заголовки безопасности: " + QString::number(res.securityHeaders.size()) + " из 5");
        }
        
        // Редирект
        if (res.hasHttpsRedirect) {
            resultsView->append("│ ✓ Редирект HTTP→HTTPS настроен");
        }
        
        // SSH версия
        if (!res.sshVersion.isEmpty()) {
            resultsView->append("│ Версия SSH: " + res.sshVersion);
        }
        
        // FTP анонимный доступ
        if (!res.ftpAnonymous.isEmpty()) {
            resultsView->append("│ " + res.ftpAnonymous);
        }
        
        // Уязвимости
        if (res.vulnerable) {
            resultsView->append("│ ┌─ НАЙДЕНА УЯЗВИМОСТЬ ──────────┐");
            resultsView->append("│ │ " + res.vulnerability.left(36));
            resultsView->append("│ └────────────────────────────────┘");
        }
        
        // Баннер
        if (!res.banner.isEmpty() && res.banner.length() < 100) {
            resultsView->append("│ Баннер: " + res.banner.left(40));
        }
        
        resultsView->append("└──────────────────────────────────────┘");
    }
    
    // Итоговая оценка
    resultsView->append("");
    resultsView->append("══════════════════════════════════════");
    if (vulnCount > 3) {
        resultsView->append("ОЦЕНКА БЕЗОПАСНОСТИ: КРИТИЧЕСКАЯ ⚠");
        resultsView->append("Обнаружено " + QString::number(vulnCount) + " уязвимостей!");
        resultsView->append("Требуется немедленное исправление!");
    } else if (vulnCount > 0) {
        resultsView->append("ОЦЕНКА БЕЗОПАСНОСТИ: СРЕДНЯЯ ⚡");
        resultsView->append("Обнаружено " + QString::number(vulnCount) + " уязвимостей.");
        resultsView->append("Рекомендуется устранить в ближайшее время.");
    } else if (openCount > 0) {
        resultsView->append("ОЦЕНКА БЕЗОПАСНОСТИ: ХОРОШАЯ ✓");
        resultsView->append("Критических уязвимостей не найдено.");
        resultsView->append("Рекомендуется регулярный мониторинг.");
    } else {
        resultsView->append("ОЦЕНКА БЕЗОПАСНОСТИ: ОТЛИЧНАЯ ✓");
        resultsView->append("Все основные порты закрыты.");
    }
    resultsView->append("══════════════════════════════════════");
    resultsView->append("\nПроверено портов: " + QString::number(results.size()));
    
    scanButton->setEnabled(true);
    scanButton->setText("Провести аудит безопасности");
    hostInput->setEnabled(true);
}

void MainWindow::onError(const QString& message) {
    resultsView->append("\n⚠ Ошибка: " + message);
    scanButton->setEnabled(true);
    scanButton->setText("Провести аудит безопасности");
    hostInput->setEnabled(true);
}
