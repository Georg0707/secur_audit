#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Аудит безопасности");
    app.setStyleSheet(
        "QMainWindow { background-color: #f5f5f5; }"
        "QGroupBox { font-weight: bold; border: 1px solid #ddd; border-radius: 5px; margin-top: 10px; padding-top: 10px; }"
    );
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
