#include "handlers.hpp"
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/utils/daemon_run.hpp>

// Optionally, we could load Qt application in a background thread if smtp_client really needs a QEventLoop, 
// but QCoreApplication app(argc, argv) is often needed for Qt network classes.
// We'll instantiate it so smtp_client can use network signals/slots.
#include <QCoreApplication>
#include <thread>
#include <QFile>
#include <QTextStream>

void loadEnv() {
  QFile file("../config.env");
  if (!file.exists())
    file.setFileName("config.env");
  if (!file.exists())
    file.setFileName("../../config.env");

  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QTextStream in(&file);
    while (!in.atEnd()) {
      QString line = in.readLine().trimmed();
      if (line.isEmpty() || line.startsWith("#"))
        continue;
      int idx = line.indexOf('=');
      if (idx != -1) {
        QString key = line.left(idx).trimmed();
        QString val = line.mid(idx + 1).trimmed();
        if (val.startsWith('"') && val.endsWith('"')) {
            val = val.mid(1, val.length() - 2);
        }
        qputenv(key.toUtf8(), val.toUtf8());
        setenv(key.toUtf8().data(), val.toUtf8().data(), 1);
      }
    }
  }
}

int main(int argc, char* argv[]) {
  // Pass env via Qt load
  QCoreApplication app(argc, argv);
  loadEnv();

  // Run Qt event loop in background for SMTP
  std::thread qt_thread([&]() {
      app.exec();
  });

  auto component_list = userver::components::MinimalServerComponentList()
                            .Append<userver::server::handlers::Ping>()
                            .Append<userver::components::Postgres>("postgres-db-1")
                            .Append<userver::clients::dns::Component>()
                            .Append<userver::components::HttpClient>()
                            // My Handlers
                            .Append<mathforces::LoginHandler>()
                            .Append<mathforces::ContestsHandler>()
                            .Append<mathforces::RatingsHandler>()
                            .Append<mathforces::ArchiveHandler>()
                            .Append<mathforces::RegisterCodeHandler>()
                            .Append<mathforces::RegisterEmailHandler>()
                            .Append<mathforces::UsersHandler>()
                            .Append<mathforces::UsersSearchHandler>()
                            .Append<mathforces::UsersProfileHandler>()
                            .Append<mathforces::UsersRoleHandler>()
                            .Append<mathforces::UsersBanHandler>()
                            .Append<mathforces::UsersBlogAccessHandler>()
                            .Append<mathforces::FriendsListHandler>()
                            .Append<mathforces::FriendsAddHandler>()
                            .Append<mathforces::FriendsRemoveHandler>()
                            .Append<mathforces::BlogPostsHandler>()
                            .Append<mathforces::BlogCommentsHandler>()
                            .Append<mathforces::CompileTypstHandler>()
                            .Append<mathforces::AdminMyContestsHandler>()
                            .Append<mathforces::AdminContestHandler>()
                            .Append<mathforces::AdminTaskHandler>()
                            .Append<mathforces::ResultsHandler>()
                            .Append<mathforces::AdminRateContestHandler>()
                            .Append<mathforces::ContestsVirtualHandler>()
                            .Append<mathforces::TasksHandler>()
                            .Append<mathforces::SubmitHandler>()
                            .Append<mathforces::SubmissionsHandler>()
                            .Append<mathforces::SubmissionsAllHandler>()
                            .Append<mathforces::HacksHandler>()
                            .Append<mathforces::OauthCallbackHandler>();

  userver::utils::DaemonMain(argc, argv, component_list);
  
  app.quit();
  qt_thread.join();
  return 0;
}
