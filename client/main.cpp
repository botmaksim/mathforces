#include "api_config.h"
#include "auth_dialog.h"
#include "main_window.h"
#include "mvc/LocalDb.h"
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
  QFile file("../config.env");
  if (!file.exists()) file.setFileName("../../config.env");
  if (!file.exists()) file.setFileName("config.env");

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
  QTextStream in(&file);
  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    if (line.isEmpty() || line.startsWith("#")) continue;
    int idx = line.indexOf('=');
    if (idx != -1) {
      QString key = line.left(idx).trimmed();
      QString val = line.mid(idx + 1).trimmed();
      if (key == "CLIENT_BASE_URL") {
        ApiConfig::baseUrl = val;
      } else if (key == "SERVER_PORT" && ApiConfig::baseUrl == "http://127.0.0.1:3000") {
        ApiConfig::baseUrl = "http://127.0.0.1:" + val;
      }
    }
  }
}

static void applyTheme(QApplication &app) {
    app.setStyle("Fusion");
    // (Убрана портянка стилей для краткости вывода, в идеале подгружать из .qss)
}

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  applyTheme(app);
  loadEnvForClient();

  // Инициализация локальной базы кэширования Qt SQL SQLite
  LocalDb::init();

  AuthDialog auth;
  if (auth.exec() == QDialog::Accepted) {
    MainWindow w(auth.getToken(), auth.getRole());
    w.resize(1280, 820);
    w.show();
    return app.exec();
  }
  return 0;
}
