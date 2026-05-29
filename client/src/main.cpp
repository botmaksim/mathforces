#include "api_config.h"
#include "app_style.h"
#include "auth_dialog.h"
#include "main_window.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>

namespace ApiConfig {
QString baseUrl = "http://127.0.0.1:3000";
}

void loadEnvForClient() {
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
    if (line.isEmpty() || line.startsWith('#'))
      continue;
    int idx = line.indexOf('=');
    if (idx == -1)
      continue;

    QString key = line.left(idx).trimmed();
    QString val = line.mid(idx + 1).trimmed();
    if (val.startsWith('"') && val.endsWith('"')) {
      val = val.mid(1, val.length() - 2);
    }
    if (key == "CLIENT_BASE_URL") {
      ApiConfig::baseUrl = val;
    } else if (key == "SERVER_PORT" && !ApiConfig::baseUrl.contains(val)) {
      ApiConfig::baseUrl = "http://127.0.0.1:" + val;
    }
  }
}

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  QApplication::setOrganizationName("MathForces");
  QApplication::setApplicationName("MathForces");
  AppStyle::applyTheme(app, AppStyle::savedTheme());
  loadEnvForClient();

  AuthDialog auth;
  if (auth.exec() == QDialog::Accepted) {
    MainWindow w(auth.getToken(), auth.getRole());
    w.show();
    return app.exec();
  }
  return 0;
}
