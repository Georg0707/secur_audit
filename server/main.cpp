#include <QCoreApplication>
#include <iostream>
#include "Server.h"

using namespace SecurityAudit;

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    std::cout << "=== Security Audit Server ===" << std::endl;
    std::cout << "Starting secure server..." << std::endl;
    
    SecureServer server;
    
    QString configFile = "../config/server.conf";
    if (argc > 1) {
        configFile = argv[1];
    }
    
    if (!server.startServer(configFile)) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    return app.exec();
}
