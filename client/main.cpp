#include "api_config.h"
#include "auth_dialog.h"
#include "main_window.h"
#include <QApplication>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QPalette>
#include <QTextStream>

namespace ApiConfig {
QString baseUrl = "http://127.0.0.1:3000";
}

void loadEnvForClient() {
  // Ищем config.env в текущей папке или выше
  QFile file("../config.env");
  if (!file.exists())
    file.setFileName("../../config.env");
  if (!file.exists())
    file.setFileName("config.env");

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return;
  }
  QTextStream in(&file);
  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    if (line.isEmpty() || line.startsWith("#"))
      continue;
    int idx = line.indexOf('=');
    if (idx != -1) {
      QString key = line.left(idx).trimmed();
      QString val = line.mid(idx + 1).trimmed();
      if (val.startsWith("\"") && val.endsWith("\"")) {
        val = val.mid(1, val.length() - 2);
      }
      if (key == "CLIENT_BASE_URL") {
        ApiConfig::baseUrl = val;
      } else if (key == "SERVER_PORT" && !ApiConfig::baseUrl.contains(val)) {
        // Пытаемся адаптировать, если CLIENT_BASE_URL не задан, но изменен
        // SERVER_PORT
        ApiConfig::baseUrl = "http://127.0.0.1:" + val;
      }
    }
  }
}

static void applyWarmLightTheme(QApplication &app) {
  app.setStyle("Fusion");
  app.setApplicationName("MathForces");
  app.setApplicationDisplayName("MathForces");
  app.setFont(QFont("Segoe UI", 10));

  QPalette palette;
  palette.setColor(QPalette::Window, QColor("#fff7ec"));
  palette.setColor(QPalette::WindowText, QColor("#3f3027"));
  palette.setColor(QPalette::Base, QColor("#fffdf8"));
  palette.setColor(QPalette::AlternateBase, QColor("#fff3e2"));
  palette.setColor(QPalette::Text, QColor("#3f3027"));
  palette.setColor(QPalette::Button, QColor("#f29965"));
  palette.setColor(QPalette::ButtonText, QColor("#ffffff"));
  palette.setColor(QPalette::Highlight, QColor("#f29965"));
  palette.setColor(QPalette::HighlightedText, QColor("#ffffff"));
  app.setPalette(palette);

  app.setStyleSheet(R"(
    * {
      selection-background-color: #f29965;
      selection-color: #ffffff;
      outline: 0;
    }

    QWidget {
      background-color: #fff7ec;
      color: #3f3027;
      font-family: "Segoe UI", "Inter", "Arial", sans-serif;
      font-size: 10.5pt;
    }

    QMainWindow, QDialog {
      background-color: #fff7ec;
    }

    QWidget#mainShell {
      background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                  stop:0 #fff7ec, stop:0.55 #fffaf3, stop:1 #fcebd7);
    }

    QFrame#topBar, QFrame#authCard, QFrame#softCard {
      background-color: rgba(255, 253, 248, 245);
      border: 1px solid #f0dec9;
      border-radius: 24px;
    }

    QLabel#brandIcon {
      background-color: #f29965;
      color: white;
      border-radius: 18px;
      min-width: 42px;
      min-height: 42px;
      max-width: 42px;
      max-height: 42px;
      font-size: 24px;
      font-weight: 800;
      qproperty-alignment: AlignCenter;
    }

    QLabel#appTitle, QLabel#authLogo {
      background: transparent;
      color: #5b341d;
      font-size: 25px;
      font-weight: 800;
      letter-spacing: 0.3px;
    }

    QLabel#authLogo {
      font-size: 30px;
    }

    QLabel#appSubtitle, QLabel#authSubtitle, QLabel#mutedLabel {
      background: transparent;
      color: #8a6b51;
      font-size: 10.5pt;
    }

    QLabel#sectionTitle {
      background: transparent;
      color: #5b341d;
      font-size: 18px;
      font-weight: 800;
    }

    QLabel#roleBadge {
      background-color: #fff0de;
      color: #8a4d20;
      border: 1px solid #f0d3b5;
      border-radius: 14px;
      padding: 8px 14px;
      font-weight: 700;
    }

    QLabel#infoCard {
      background-color: #fffdf8;
      border: 1px solid #f0dec9;
      border-radius: 18px;
      padding: 16px;
      color: #5a4636;
    }

    QTabWidget#mainTabs::pane {
      border: none;
      background: transparent;
      margin-top: 12px;
    }

    QTabBar::tab {
      background-color: #fff3e2;
      color: #785338;
      border: 1px solid #efd8bc;
      border-radius: 16px;
      padding: 10px 18px;
      margin-right: 8px;
      min-width: 112px;
      font-weight: 650;
    }

    QTabBar::tab:selected {
      background-color: #f29965;
      color: #ffffff;
      border-color: #f29965;
    }

    QTabBar::tab:hover:!selected {
      background-color: #ffe7ca;
      border-color: #e8c39d;
    }

    QPushButton {
      background-color: #f29965;
      color: white;
      border: none;
      border-radius: 15px;
      padding: 10px 16px;
      min-height: 22px;
      font-weight: 700;
    }

    QPushButton:hover {
      background-color: #e9824a;
    }

    QPushButton:pressed {
      background-color: #cf6f38;
      padding-top: 11px;
      padding-bottom: 9px;
    }

    QPushButton:disabled {
      background-color: #ead8c7;
      color: #ad9886;
    }

    QPushButton:flat {
      background: transparent;
      color: #b56832;
      border: none;
      font-weight: 700;
      padding: 6px;
    }

    QPushButton:flat:hover {
      color: #8d461c;
      text-decoration: underline;
    }

    QLineEdit, QTextEdit, QPlainTextEdit, QComboBox, QDateTimeEdit, QDoubleSpinBox, QSpinBox {
      background-color: #fffdf8;
      color: #3f3027;
      border: 1px solid #ecd7bf;
      border-radius: 14px;
      padding: 9px 12px;
    }

    QTextEdit, QPlainTextEdit {
      line-height: 130%;
    }

    QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QComboBox:focus,
    QDateTimeEdit:focus, QDoubleSpinBox:focus, QSpinBox:focus {
      border: 2px solid #f29965;
      padding: 8px 11px;
      background-color: #ffffff;
    }

    QComboBox::drop-down, QDateTimeEdit::drop-down {
      border: none;
      width: 28px;
    }

    QListWidget, QTableWidget, QTreeWidget {
      background-color: #fffdf8;
      alternate-background-color: #fff5e8;
      border: 1px solid #f0dec9;
      border-radius: 18px;
      padding: 8px;
      gridline-color: #f3dfc9;
    }

    QListWidget::item {
      padding: 11px 12px;
      margin: 4px;
      border-radius: 13px;
      color: #4d392d;
    }

    QListWidget::item:hover {
      background-color: #fff0de;
    }

    QListWidget::item:selected {
      background-color: #f29965;
      color: #ffffff;
    }

    QTableWidget::item {
      padding: 7px;
      border-radius: 8px;
    }

    QTableWidget::item:selected {
      background-color: #f29965;
      color: #ffffff;
    }

    QHeaderView::section {
      background-color: #ffe7ca;
      color: #6d452d;
      border: none;
      border-right: 1px solid #f0dec9;
      border-bottom: 1px solid #f0dec9;
      padding: 9px;
      font-weight: 800;
    }

    QGroupBox {
      background-color: #fffdf8;
      border: 1px solid #f0dec9;
      border-radius: 20px;
      margin-top: 18px;
      padding: 16px 14px 14px 14px;
      color: #5b341d;
      font-weight: 800;
    }

    QGroupBox::title {
      subcontrol-origin: margin;
      left: 18px;
      padding: 2px 8px;
      color: #7b4a25;
      background-color: #fff7ec;
      border-radius: 8px;
    }

    QCheckBox {
      background: transparent;
      spacing: 8px;
      color: #5a4636;
    }

    QCheckBox::indicator {
      width: 18px;
      height: 18px;
      border-radius: 6px;
      border: 1px solid #d6b898;
      background: #fffdf8;
    }

    QCheckBox::indicator:checked {
      background-color: #f29965;
      border-color: #f29965;
    }

    QScrollBar:vertical {
      background: transparent;
      width: 12px;
      margin: 8px 0 8px 0;
    }

    QScrollBar::handle:vertical {
      background: #e7c8aa;
      border-radius: 6px;
      min-height: 32px;
    }

    QScrollBar::handle:vertical:hover {
      background: #d9aa80;
    }

    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
      height: 0;
      background: transparent;
    }

    QScrollBar:horizontal {
      background: transparent;
      height: 12px;
      margin: 0 8px 0 8px;
    }

    QScrollBar::handle:horizontal {
      background: #e7c8aa;
      border-radius: 6px;
      min-width: 32px;
    }

    QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
    QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
      width: 0;
      background: transparent;
    }

    QPdfView {
      background-color: #fffdf8;
      border: 1px solid #f0dec9;
      border-radius: 18px;
    }

    QMessageBox QLabel {
      background: transparent;
    }
  )");
}

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  applyWarmLightTheme(app);

  loadEnvForClient();

  AuthDialog auth;
  if (auth.exec() == QDialog::Accepted) {
    MainWindow w(auth.getToken(), auth.getRole());
    w.resize(1280, 820);
    w.show();
    return app.exec();
  }
  return 0;
}
