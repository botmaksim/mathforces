#include "app_style.h"

#include <QColor>
#include <QFont>
#include <QPalette>
#include <QSettings>

namespace {
constexpr auto kSettingsGroup = "ui";
constexpr auto kThemeKey = "theme";

QString lightStyleSheet() {
  return QStringLiteral(R"(
    * {
      selection-background-color: #2f5fc6;
      selection-color: #ffffff;
      outline: 0;
    }

    QWidget {
      background-color: #f5f7fb;
      color: #1f2d3d;
      font-family: "Segoe UI", "Inter", "Arial", sans-serif;
      font-size: 10.5pt;
    }

    QMainWindow, QDialog {
      background-color: #f5f7fb;
    }

    QWidget#mainShell {
      background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                  stop:0 #eef4ff, stop:0.52 #ffffff, stop:1 #f6edff);
    }

    QFrame#topBar, QFrame#authCard, QFrame#softCard, QFrame#taskPanel,
    QFrame#welcomeHero, QFrame#archiveTaskCard {
      background-color: rgba(255, 255, 255, 248);
      border: 1px solid #d7e1f2;
      border-radius: 22px;
    }

    QLabel#brandIcon {
      background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                  stop:0 #1846a3, stop:0.55 #5a2fd1, stop:1 #02bfd7);
      color: white;
      border-radius: 18px;
      min-width: 44px;
      min-height: 44px;
      max-width: 44px;
      max-height: 44px;
      font-size: 24px;
      font-weight: 900;
      qproperty-alignment: AlignCenter;
    }

    QLabel#appTitle, QLabel#authLogo {
      background: transparent;
      color: #13265c;
      font-size: 25px;
      font-weight: 900;
      letter-spacing: 0.4px;
    }

    QLabel#authLogo {
      font-size: 31px;
    }

    QLabel#appSubtitle, QLabel#authSubtitle, QLabel#mutedLabel,
    QLabel#emptyHint {
      background: transparent;
      color: #61708a;
      font-size: 10.5pt;
    }

    QLabel#sectionTitle {
      background: transparent;
      color: #13265c;
      font-size: 18px;
      font-weight: 900;
    }

    QLabel#welcomeTitle {
      background: transparent;
      color: #13265c;
      font-size: 32px;
      font-weight: 900;
    }

    QLabel#roleBadge {
      background-color: #eaf1ff;
      color: #1846a3;
      border: 1px solid #cddaf2;
      border-radius: 14px;
      padding: 8px 14px;
      font-weight: 800;
    }

    QLabel#infoCard, QTextEdit#readOnlyCard {
      background-color: #ffffff;
      border: 1px solid #d7e1f2;
      border-radius: 18px;
      padding: 16px;
      color: #26384e;
    }

    QTabWidget#mainTabs::pane {
      border: none;
      background: transparent;
      margin-top: 12px;
    }

    QTabBar::tab {
      background-color: #ffffff;
      color: #2a3c55;
      border: 1px solid #d7e1f2;
      border-radius: 15px;
      padding: 10px 16px;
      margin-right: 7px;
      min-width: 104px;
      font-weight: 750;
    }

    QTabBar::tab:selected {
      background-color: #265fcf;
      color: #ffffff;
      border-color: #265fcf;
    }

    QTabBar::tab:hover:!selected {
      background-color: #edf4ff;
      border-color: #bcd0f4;
    }

    QPushButton {
      background-color: #265fcf;
      color: white;
      border: none;
      border-radius: 14px;
      padding: 10px 16px;
      min-height: 22px;
      font-weight: 800;
    }

    QPushButton:hover { background-color: #1e4fae; }
    QPushButton:pressed { background-color: #183f8e; padding-top: 11px; padding-bottom: 9px; }
    QPushButton:disabled { background-color: #d8dfeb; color: #8997ad; }

    QPushButton#secondaryButton, QPushButton:flat {
      background: transparent;
      color: #265fcf;
      border: 1px solid #cbd9f1;
      font-weight: 800;
      padding: 8px 12px;
    }

    QPushButton#secondaryButton:hover, QPushButton:flat:hover {
      background-color: #edf4ff;
      text-decoration: none;
    }

    QPushButton#themeToggle {
      background-color: #f0f4fb;
      color: #183f8e;
      border: 1px solid #cbd9f1;
    }

    QLineEdit, QTextEdit, QPlainTextEdit, QComboBox, QDateTimeEdit, QDoubleSpinBox, QSpinBox {
      background-color: #ffffff;
      color: #1f2d3d;
      border: 1px solid #d4deef;
      border-radius: 13px;
      padding: 9px 12px;
    }

    QTextEdit, QPlainTextEdit { line-height: 130%; }

    QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QComboBox:focus,
    QDateTimeEdit:focus, QDoubleSpinBox:focus, QSpinBox:focus {
      border: 2px solid #265fcf;
      padding: 8px 11px;
      background-color: #ffffff;
    }

    QComboBox::drop-down, QDateTimeEdit::drop-down { border: none; width: 28px; }

    QListWidget, QTableWidget, QTreeWidget {
      background-color: #ffffff;
      alternate-background-color: #f4f8ff;
      border: 1px solid #d7e1f2;
      border-radius: 18px;
      padding: 8px;
      gridline-color: #e1e8f5;
    }

    QListWidget::item {
      padding: 11px 12px;
      margin: 4px;
      border-radius: 13px;
      color: #26384e;
    }

    QListWidget::item:hover { background-color: #edf4ff; }
    QListWidget::item:selected { background-color: #265fcf; color: #ffffff; }

    QTableWidget::item { padding: 7px; border-radius: 8px; }
    QTableWidget::item:selected { background-color: #265fcf; color: #ffffff; }

    QHeaderView::section {
      background-color: #eaf1ff;
      color: #13265c;
      border: none;
      border-right: 1px solid #d7e1f2;
      border-bottom: 1px solid #d7e1f2;
      padding: 9px;
      font-weight: 900;
    }

    QGroupBox {
      background-color: #ffffff;
      border: 1px solid #d7e1f2;
      border-radius: 20px;
      margin-top: 18px;
      padding: 16px 14px 14px 14px;
      color: #13265c;
      font-weight: 900;
    }

    QGroupBox::title {
      subcontrol-origin: margin;
      left: 18px;
      padding: 2px 8px;
      color: #1846a3;
      background-color: #f5f7fb;
      border-radius: 8px;
    }

    QCheckBox { background: transparent; spacing: 8px; color: #26384e; }
    QCheckBox::indicator {
      width: 18px;
      height: 18px;
      border-radius: 6px;
      border: 1px solid #bdcadf;
      background: #ffffff;
    }
    QCheckBox::indicator:checked { background-color: #265fcf; border-color: #265fcf; }

    QSplitter::handle {
      background-color: rgba(38, 95, 207, 0.12);
      border-radius: 5px;
      margin: 4px;
    }

    QScrollBar:vertical { background: transparent; width: 12px; margin: 8px 0 8px 0; }
    QScrollBar::handle:vertical { background: #c2cde0; border-radius: 6px; min-height: 32px; }
    QScrollBar::handle:vertical:hover { background: #aab8cf; }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { height: 0; background: transparent; }
    QScrollBar:horizontal { background: transparent; height: 12px; margin: 0 8px 0 8px; }
    QScrollBar::handle:horizontal { background: #c2cde0; border-radius: 6px; min-width: 32px; }
    QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
    QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { width: 0; background: transparent; }

    QPdfView { background-color: #ffffff; border: 1px solid #d7e1f2; border-radius: 18px; }
    QMessageBox QLabel { background: transparent; }

    QLabel#toast {
      background-color: #143d8c;
      color: #ffffff;
      border-radius: 16px;
      padding: 12px 18px;
      font-weight: 800;
    }
  )");
}

QString darkStyleSheet() {
  return QStringLiteral(R"(
    * {
      selection-background-color: #2dd4ff;
      selection-color: #07111f;
      outline: 0;
    }

    QWidget {
      background-color: #09111f;
      color: #edf5ff;
      font-family: "Segoe UI", "Inter", "Arial", sans-serif;
      font-size: 10.5pt;
    }

    QMainWindow, QDialog { background-color: #09111f; }

    QWidget#mainShell {
      background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                  stop:0 #07111f, stop:0.55 #101a2f, stop:1 #1d1232);
    }

    QFrame#topBar, QFrame#authCard, QFrame#softCard, QFrame#taskPanel,
    QFrame#welcomeHero, QFrame#archiveTaskCard {
      background-color: rgba(18, 29, 52, 245);
      border: 1px solid #263757;
      border-radius: 22px;
    }

    QLabel#brandIcon {
      background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                  stop:0 #0f3f95, stop:0.55 #772ce8, stop:1 #00c6df);
      color: white;
      border-radius: 18px;
      min-width: 44px;
      min-height: 44px;
      max-width: 44px;
      max-height: 44px;
      font-size: 24px;
      font-weight: 900;
      qproperty-alignment: AlignCenter;
    }

    QLabel#appTitle, QLabel#authLogo, QLabel#sectionTitle, QLabel#welcomeTitle {
      background: transparent;
      color: #f4f8ff;
      font-weight: 900;
    }
    QLabel#appTitle, QLabel#authLogo { font-size: 25px; letter-spacing: 0.4px; }
    QLabel#authLogo { font-size: 31px; }
    QLabel#sectionTitle { font-size: 18px; }
    QLabel#welcomeTitle { font-size: 32px; }

    QLabel#appSubtitle, QLabel#authSubtitle, QLabel#mutedLabel,
    QLabel#emptyHint {
      background: transparent;
      color: #9fb0ca;
      font-size: 10.5pt;
    }

    QLabel#roleBadge {
      background-color: #182849;
      color: #79dfff;
      border: 1px solid #2d4f7e;
      border-radius: 14px;
      padding: 8px 14px;
      font-weight: 800;
    }

    QLabel#infoCard, QTextEdit#readOnlyCard {
      background-color: #121d34;
      border: 1px solid #263757;
      border-radius: 18px;
      padding: 16px;
      color: #edf5ff;
    }

    QTabWidget#mainTabs::pane { border: none; background: transparent; margin-top: 12px; }
    QTabBar::tab {
      background-color: #121d34;
      color: #b5c3d8;
      border: 1px solid #263757;
      border-radius: 15px;
      padding: 10px 16px;
      margin-right: 7px;
      min-width: 104px;
      font-weight: 750;
    }
    QTabBar::tab:selected { background-color: #2dd4ff; color: #07111f; border-color: #2dd4ff; }
    QTabBar::tab:hover:!selected { background-color: #182849; border-color: #365b8d; }

    QPushButton {
      background-color: #2dd4ff;
      color: #07111f;
      border: none;
      border-radius: 14px;
      padding: 10px 16px;
      min-height: 22px;
      font-weight: 900;
    }
    QPushButton:hover { background-color: #79e3ff; }
    QPushButton:pressed { background-color: #1aa9ce; padding-top: 11px; padding-bottom: 9px; }
    QPushButton:disabled { background-color: #263757; color: #71829c; }

    QPushButton#secondaryButton, QPushButton:flat {
      background: transparent;
      color: #79dfff;
      border: 1px solid #2d4f7e;
      font-weight: 800;
      padding: 8px 12px;
    }
    QPushButton#secondaryButton:hover, QPushButton:flat:hover { background-color: #182849; text-decoration: none; }
    QPushButton#themeToggle { background-color: #182849; color: #79dfff; border: 1px solid #2d4f7e; }

    QLineEdit, QTextEdit, QPlainTextEdit, QComboBox, QDateTimeEdit, QDoubleSpinBox, QSpinBox {
      background-color: #0d1729;
      color: #edf5ff;
      border: 1px solid #263757;
      border-radius: 13px;
      padding: 9px 12px;
    }
    QTextEdit, QPlainTextEdit { line-height: 130%; }
    QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QComboBox:focus,
    QDateTimeEdit:focus, QDoubleSpinBox:focus, QSpinBox:focus {
      border: 2px solid #2dd4ff;
      padding: 8px 11px;
      background-color: #111c32;
    }
    QComboBox::drop-down, QDateTimeEdit::drop-down { border: none; width: 28px; }

    QListWidget, QTableWidget, QTreeWidget {
      background-color: #0d1729;
      alternate-background-color: #121d34;
      border: 1px solid #263757;
      border-radius: 18px;
      padding: 8px;
      gridline-color: #22314f;
    }
    QListWidget::item { padding: 11px 12px; margin: 4px; border-radius: 13px; color: #edf5ff; }
    QListWidget::item:hover { background-color: #182849; }
    QListWidget::item:selected { background-color: #2dd4ff; color: #07111f; }
    QTableWidget::item { padding: 7px; border-radius: 8px; }
    QTableWidget::item:selected { background-color: #2dd4ff; color: #07111f; }
    QHeaderView::section {
      background-color: #182849;
      color: #edf5ff;
      border: none;
      border-right: 1px solid #263757;
      border-bottom: 1px solid #263757;
      padding: 9px;
      font-weight: 900;
    }

    QGroupBox {
      background-color: #121d34;
      border: 1px solid #263757;
      border-radius: 20px;
      margin-top: 18px;
      padding: 16px 14px 14px 14px;
      color: #edf5ff;
      font-weight: 900;
    }
    QGroupBox::title {
      subcontrol-origin: margin;
      left: 18px;
      padding: 2px 8px;
      color: #79dfff;
      background-color: #09111f;
      border-radius: 8px;
    }

    QCheckBox { background: transparent; spacing: 8px; color: #edf5ff; }
    QCheckBox::indicator { width: 18px; height: 18px; border-radius: 6px; border: 1px solid #365b8d; background: #0d1729; }
    QCheckBox::indicator:checked { background-color: #2dd4ff; border-color: #2dd4ff; }
    QSplitter::handle { background-color: rgba(45, 212, 255, 0.20); border-radius: 5px; margin: 4px; }

    QScrollBar:vertical { background: transparent; width: 12px; margin: 8px 0 8px 0; }
    QScrollBar::handle:vertical { background: #314467; border-radius: 6px; min-height: 32px; }
    QScrollBar::handle:vertical:hover { background: #3d587f; }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { height: 0; background: transparent; }
    QScrollBar:horizontal { background: transparent; height: 12px; margin: 0 8px 0 8px; }
    QScrollBar::handle:horizontal { background: #314467; border-radius: 6px; min-width: 32px; }
    QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
    QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { width: 0; background: transparent; }

    QPdfView { background-color: #0d1729; border: 1px solid #263757; border-radius: 18px; }
    QMessageBox QLabel { background: transparent; }
    QLabel#toast { background-color: #2dd4ff; color: #07111f; border-radius: 16px; padding: 12px 18px; font-weight: 900; }
  )");
}

void configurePalette(QApplication &app, AppStyle::Theme theme) {
  QPalette palette;
  if (theme == AppStyle::Theme::Dark) {
    palette.setColor(QPalette::Window, QColor("#09111f"));
    palette.setColor(QPalette::WindowText, QColor("#edf5ff"));
    palette.setColor(QPalette::Base, QColor("#0d1729"));
    palette.setColor(QPalette::AlternateBase, QColor("#121d34"));
    palette.setColor(QPalette::Text, QColor("#edf5ff"));
    palette.setColor(QPalette::Button, QColor("#2dd4ff"));
    palette.setColor(QPalette::ButtonText, QColor("#07111f"));
    palette.setColor(QPalette::Highlight, QColor("#2dd4ff"));
    palette.setColor(QPalette::HighlightedText, QColor("#07111f"));
    palette.setColor(QPalette::PlaceholderText, QColor("#7385a1"));
  } else {
    palette.setColor(QPalette::Window, QColor("#f5f7fb"));
    palette.setColor(QPalette::WindowText, QColor("#1f2d3d"));
    palette.setColor(QPalette::Base, QColor("#ffffff"));
    palette.setColor(QPalette::AlternateBase, QColor("#f4f8ff"));
    palette.setColor(QPalette::Text, QColor("#1f2d3d"));
    palette.setColor(QPalette::Button, QColor("#265fcf"));
    palette.setColor(QPalette::ButtonText, QColor("#ffffff"));
    palette.setColor(QPalette::Highlight, QColor("#265fcf"));
    palette.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    palette.setColor(QPalette::PlaceholderText, QColor("#7b8ba3"));
  }
  app.setPalette(palette);
}
} // namespace

namespace AppStyle {

Theme savedTheme() {
  QSettings settings;
  settings.beginGroup(kSettingsGroup);
  const QString value = settings.value(kThemeKey, "light").toString();
  settings.endGroup();
  return value == "dark" ? Theme::Dark : Theme::Light;
}

void saveTheme(Theme theme) {
  QSettings settings;
  settings.beginGroup(kSettingsGroup);
  settings.setValue(kThemeKey, theme == Theme::Dark ? "dark" : "light");
  settings.endGroup();
}

void applyTheme(QApplication &app, Theme theme) {
  app.setStyle("Fusion");
  app.setApplicationName("MathForces");
  app.setApplicationDisplayName("MathForces");
  app.setOrganizationName("MathForces");
  app.setFont(QFont("Segoe UI", 10));
  configurePalette(app, theme);
  app.setStyleSheet(theme == Theme::Dark ? darkStyleSheet() : lightStyleSheet());
}

Theme toggleTheme(QApplication &app) {
  Theme next = savedTheme() == Theme::Dark ? Theme::Light : Theme::Dark;
  saveTheme(next);
  applyTheme(app, next);
  return next;
}

QString themeLabel(Theme theme) {
  return theme == Theme::Dark ? QStringLiteral("Тёмная") : QStringLiteral("Светлая");
}

QIcon standardIcon(QStyle::StandardPixmap pixmap) {
  return QApplication::style()->standardIcon(pixmap);
}

} // namespace AppStyle
