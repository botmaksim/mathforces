#include <QApplication>
#include "auth_dialog.h"
#include "main_window.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    // Светлый/Темный минималистичный стиль QSS
    app.setStyleSheet(R"(
        QWidget { background-color: #2b2b2b; color: #e0e0e0; font-family: 'Segoe UI', sans-serif; font-size: 14pt; }
        QPushButton { background-color: #007acc; color: white; padding: 6px 12px; border: none; border-radius: 4px; }
        QPushButton:hover { background-color: #0098ff; }
        QLineEdit, QTextEdit { background-color: #3c3f41; border: 1px solid #555; border-radius: 3px; padding: 4px; }
        QTabWidget::pane { border: 1px solid #444; }
        QTabBar::tab { background: #3c3f41; padding: 8px 16px; margin-right: 2px; }
        QTabBar::tab:selected { background: #007acc; color: white; }
        QTableWidget { background-color: #3c3f41; alternate-background-color: #323232; }
        QHeaderView::section { background-color: #444; padding: 4px; }
    )");

    AuthDialog auth;
    if (auth.exec() == QDialog::Accepted) {
        MainWindow w(auth.getToken(), auth.getRole());
        w.resize(1024, 768);
        w.show();
        return app.exec();
    }
    return 0;
}
