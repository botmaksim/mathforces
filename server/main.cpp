#include <userver/components/minimal_server_component_list.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/clients/dns/component.hpp>

#include "handlers.hpp"

int main(int argc, char* argv[]) {
  auto component_list = userver::components::MinimalServerComponentList()
                            .Append<userver::server::handlers::Ping>()
                            .Append<userver::components::TestsuiteSupport>()
                            .Append<userver::server::handlers::TestsControl>()
                            .Append<userver::components::HttpClient>()
                            .Append<userver::clients::dns::Component>()
                            .Append<userver::components::Postgres>("postgres-db-1")
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

  return userver::utils::DaemonMain(argc, argv, component_list);
}
