#pragma once
#include <userver/components/component_list.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

namespace mathforces {

class LoginHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-login";
  LoginHandler(const userver::components::ComponentConfig& config,
               const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class ContestsHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-contests";
  ContestsHandler(const userver::components::ComponentConfig& config,
                  const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class RatingsHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-ratings";
  RatingsHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class ArchiveHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-archive";
  ArchiveHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};


class RegisterCodeHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-registercode";
  RegisterCodeHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class RegisterEmailHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-registeremail";
  RegisterEmailHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class UsersHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-users";
  UsersHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class UsersSearchHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-userssearch";
  UsersSearchHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class UsersProfileHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-usersprofile";
  UsersProfileHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class UsersRoleHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-usersrole";
  UsersRoleHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class UsersBanHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-usersban";
  UsersBanHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class UsersBlogAccessHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-usersblogaccess";
  UsersBlogAccessHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class FriendsListHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-friendslist";
  FriendsListHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class FriendsAddHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-friendsadd";
  FriendsAddHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class FriendsRemoveHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-friendsremove";
  FriendsRemoveHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class BlogPostsHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-blogposts";
  BlogPostsHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class BlogCommentsHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-blogcomments";
  BlogCommentsHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class CompileTypstHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-compiletypst";
  CompileTypstHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class AdminMyContestsHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-adminmycontests";
  AdminMyContestsHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class AdminContestHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-admincontest";
  AdminContestHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class AdminTaskHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-admintask";
  AdminTaskHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class ResultsHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-results";
  ResultsHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class AdminRateContestHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-adminratecontest";
  AdminRateContestHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class ContestsVirtualHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-contestsvirtual";
  ContestsVirtualHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class TasksHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-tasks";
  TasksHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class SubmitHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-submit";
  SubmitHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class SubmissionsHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-submissions";
  SubmissionsHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class SubmissionsAllHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-submissionsall";
  SubmissionsAllHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class HacksHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-hacks";
  HacksHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class OauthCallbackHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-oauthcallback";
  OauthCallbackHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override;
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

} // namespace mathforces
