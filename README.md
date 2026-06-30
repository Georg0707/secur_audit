# Security Audit – Клиент-серверный аудит безопасности

[![CI/CD Pipeline](https://github.com/YOUR_USERNAME/YOUR_REPO/actions/workflows/ci.yml/badge.svg)](https://github.com/YOUR_USERNAME/YOUR_REPO/actions/workflows/ci.yml)

## Описание

Клиент-серверное приложение для аудита безопасности сетевых сервисов. Выполняет TCP-сканирование портов, анализ SSL/TLS, проверку HTTP-заголовков безопасности и поиск уязвимостей. Поддерживает до 4 клиентов одновременно через защищенное TLS-соединение.

## Стек технологий

- **Backend**: C++17, Qt6, OpenSSL, многопоточность
- **Frontend**: Qt6 Widgets (GUI) / консольный режим
- **Сборка**: CMake 3.23+, Make, Docker
- **CI/CD**: GitHub Actions
- **Контейнеризация**: Docker, Docker Compose
- **Оркестрация**: Kubernetes (опционально)

## Быстрый старт

### Локальный запуск (Fedora)

```bash
sudo dnf install qt6-qtbase-devel qt6-qtbase-gui cmake gcc-c++ openssl
cd certs && chmod +x generate_certs.sh && ./generate_certs.sh
cd .. && make build
./bin/server config/server.conf     # Терминал 1
./bin/client_gui                     # Терминал 2
