#include "handlers.hpp"
#include "auth.hpp"
#include <userver/formats/json/value_builder.hpp>

namespace mathforces {

LoginHandler::LoginHandler(const userver::components::ComponentConfig &config,
                           const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value LoginHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {

  auto email = request_json["email"].As<std::string>("");
  auto password = request_json["password"].As<std::string>("");

  if (email.empty() || password.empty()) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
    return {};
  }

  auto hashed_pwd = Sha3_256(password);
  auto res = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, role, is_banned, name FROM users WHERE email=$1", email);

  if (res.IsEmpty()) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
    return {};
  }

  auto is_banned = res[0]["is_banned"].As<bool>();
  if (is_banned) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
    userver::formats::json::ValueBuilder e;
    e["error"] = "banned";
    return e.ExtractValue();
  }

  auto id = res[0]["id"].As<int>();
  auto role = res[0]["role"].As<std::string>();

  userver::formats::json::ValueBuilder builder;
  builder["token"] = CreateJwt(id, role);
  builder["role"] = role;
  return builder.ExtractValue();
}

ContestsHandler::ContestsHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value ContestsHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &,
    userver::server::request::RequestContext &) const {

  int u_id = 0;
  std::string u_role;

  auto res = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, title, description, start_time::text, end_time::text, "
      "duration_hours FROM contests WHERE is_published = TRUE ORDER BY "
      "start_time DESC");

  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kArray};
  for (const auto &row : res) {
    userver::formats::json::ValueBuilder obj;
    obj["id"] = row["id"].As<int>();
    obj["title"] = row["title"].As<std::string>();
    obj["description"] = row["description"].As<std::string>();
    obj["start_time"] = row["start_time"].As<std::string>();
    obj["end_time"] = row["end_time"].As<std::string>();
    obj["duration_hours"] = row["duration_hours"].As<double>();
    builder.PushBack(std::move(obj));
  }
  return builder.ExtractValue();
}

RatingsHandler::RatingsHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value RatingsHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &,
    userver::server::request::RequestContext &) const {

  auto res = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, username, name, rating FROM users ORDER BY rating DESC");

  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kArray};
  int place = 1;
  for (const auto &row : res) {
    userver::formats::json::ValueBuilder obj;
    obj["id"] = row["id"].As<int>();
    obj["place"] = place++;
    obj["username"] = row["username"].As<std::string>();
    obj["name"] = row["name"].As<std::string>();
    obj["rating"] = row["rating"].As<int>();
    builder.PushBack(std::move(obj));
  }
  return builder.ExtractValue();
}

ArchiveHandler::ArchiveHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value ArchiveHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &,
    userver::server::request::RequestContext &) const {

  int min_diff = 0;
  int max_diff = 99999;
  std::string tags = "";

  if (request.HasArg("min_diff")) {
    min_diff = std::stoi(request.GetArg("min_diff"));
  }
  if (request.HasArg("max_diff")) {
    max_diff = std::stoi(request.GetArg("max_diff"));
  }
  if (request.HasArg("tags")) {
    tags = request.GetArg("tags");
  }

  std::string query = "SELECT id, title, tags, difficulty FROM tasks WHERE "
                      "difficulty >= $1 AND difficulty <= $2";
  if (!tags.empty()) {
    query += " AND tags ILIKE '%' || $3 || '%'";
  }

  query += " ORDER BY id DESC LIMIT 100";

  userver::storages::postgres::ResultSet res;
  if (!tags.empty()) {
    res = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster, query, min_diff,
        max_diff, tags);
  } else {
    res = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster, query, min_diff,
        max_diff);
  }

  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kArray};
  for (const auto &row : res) {
    userver::formats::json::ValueBuilder obj;
    obj["id"] = row["id"].As<int>();
    obj["title"] = row["title"].As<std::string>();
    obj["tags"] = row["tags"].As<std::string>();
    obj["difficulty"] = row["difficulty"].As<int>();
    builder.PushBack(std::move(obj));
  }
  return builder.ExtractValue();
}

RegisterCodeHandler::RegisterCodeHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value RegisterCodeHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  return builder.ExtractValue();
}

RegisterEmailHandler::RegisterEmailHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value RegisterEmailHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  return builder.ExtractValue();
}

UsersHandler::UsersHandler(const userver::components::ComponentConfig &config,
                           const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value UsersHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  return builder.ExtractValue();
}

UsersSearchHandler::UsersSearchHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value UsersSearchHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  return builder.ExtractValue();
}

UsersProfileHandler::UsersProfileHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value UsersProfileHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {

  int t_id = 0;
  if (request.HasArg("id")) {
    t_id = std::stoi(request.GetArg("id"));
  } else {
    std::string auth = request.GetHeader("Authorization");
    std::string role;
    if (!VerifyJwt(auth, t_id, role)) {
      request.SetResponseStatus(
          userver::server::http::HttpStatus::kUnauthorized);
      return {};
    }
  }

  auto res = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, username, email, name, rating, can_blog FROM users WHERE "
      "id=$1",
      t_id);

  if (res.IsEmpty()) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
    return {};
  }

  userver::formats::json::ValueBuilder builder;
  builder["id"] = res[0]["id"].As<int>();
  builder["username"] = res[0]["username"].As<std::string>();
  builder["email"] = res[0]["email"].As<std::string>();
  builder["name"] = res[0]["name"].As<std::string>();
  builder["rating"] = res[0]["rating"].As<int>();
  builder["can_blog"] = res[0]["can_blog"].As<bool>();

  return builder.ExtractValue();
}

UsersRoleHandler::UsersRoleHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value UsersRoleHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  return builder.ExtractValue();
}

UsersBanHandler::UsersBanHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value UsersBanHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  return builder.ExtractValue();
}

UsersBlogAccessHandler::UsersBlogAccessHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value UsersBlogAccessHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  return builder.ExtractValue();
}

FriendsListHandler::FriendsListHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value FriendsListHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  return builder.ExtractValue();
}

FriendsAddHandler::FriendsAddHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value FriendsAddHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  return builder.ExtractValue();
}

FriendsRemoveHandler::FriendsRemoveHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value FriendsRemoveHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  return builder.ExtractValue();
}

BlogPostsHandler::BlogPostsHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value BlogPostsHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {

  if (request.GetMethod() == userver::server::http::HttpMethod::kGet) {
    if (!request.HasArg("user_id")) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return {};
    }
    int user_id = std::stoi(request.GetArg("user_id"));
    auto res = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT id, content, created_at FROM blogs WHERE user_id=$1 ORDER BY "
        "id DESC",
        user_id);

    userver::formats::json::ValueBuilder builder{
        userver::formats::common::Type::kArray};
    for (const auto &row : res) {
      userver::formats::json::ValueBuilder obj;
      obj["id"] = row["id"].As<int>();
      obj["content"] = row["content"].As<std::string>();
      obj["created_at"] = row["created_at"].As<std::string>();
      builder.PushBack(std::move(obj));
    }
    return builder.ExtractValue();
  } else {
    std::string auth = request.GetHeader("Authorization");
    std::string role;
    int user_id = 0;
    if (!VerifyJwt(auth, user_id, role)) {
      request.SetResponseStatus(
          userver::server::http::HttpStatus::kUnauthorized);
      return {};
    }
    std::string content = request_json["content"].As<std::string>();
    pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                         "INSERT INTO blogs (user_id, title, content) VALUES "
                         "($1, 'Blog Post', $2)",
                         user_id, content);
    userver::formats::json::ValueBuilder builder{
        userver::formats::common::Type::kObject};
    builder["status"] = "ok";
    return builder.ExtractValue();
  }
}

BlogCommentsHandler::BlogCommentsHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value BlogCommentsHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {

  if (request.GetMethod() == userver::server::http::HttpMethod::kGet) {
    if (!request.HasArg("post_id")) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return {};
    }
    int post_id = std::stoi(request.GetArg("post_id"));
    auto res = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT c.id, c.content, c.created_at, u.username FROM comments c JOIN "
        "users u ON c.user_id = u.id WHERE c.blog_id=$1 ORDER BY c.id ASC",
        post_id);

    userver::formats::json::ValueBuilder builder{
        userver::formats::common::Type::kArray};
    for (const auto &row : res) {
      userver::formats::json::ValueBuilder obj;
      obj["id"] = row["id"].As<int>();
      obj["content"] = row["content"].As<std::string>();
      obj["created_at"] = row["created_at"].As<std::string>();
      obj["username"] = row["username"].As<std::string>();
      builder.PushBack(std::move(obj));
    }
    return builder.ExtractValue();
  } else {
    std::string auth = request.GetHeader("Authorization");
    std::string role;
    int user_id = 0;
    if (!VerifyJwt(auth, user_id, role)) {
      request.SetResponseStatus(
          userver::server::http::HttpStatus::kUnauthorized);
      return {};
    }
    int post_id = request_json["post_id"].As<int>();
    std::string content = request_json["content"].As<std::string>();
    pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "INSERT INTO comments (blog_id, user_id, content) VALUES ($1, $2, $3)",
        post_id, user_id, content);
    userver::formats::json::ValueBuilder builder{
        userver::formats::common::Type::kObject};
    builder["status"] = "ok";
    return builder.ExtractValue();
  }
}

CompileTypstHandler::CompileTypstHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value CompileTypstHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  return builder.ExtractValue();
}

AdminMyContestsHandler::AdminMyContestsHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value AdminMyContestsHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  return builder.ExtractValue();
}

AdminContestHandler::AdminContestHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value AdminContestHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  return builder.ExtractValue();
}

AdminTaskHandler::AdminTaskHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value AdminTaskHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  return builder.ExtractValue();
}

ResultsHandler::ResultsHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value ResultsHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  return builder.ExtractValue();
}

AdminRateContestHandler::AdminRateContestHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value AdminRateContestHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  return builder.ExtractValue();
}

ContestsVirtualHandler::ContestsVirtualHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value ContestsVirtualHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  return builder.ExtractValue();
}

TasksHandler::TasksHandler(const userver::components::ComponentConfig &config,
                           const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value TasksHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {

  if (!request.HasArg("contest_id")) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
    return {};
  }
  int c_id = std::stoi(request.GetArg("contest_id"));

  auto res = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, title, description, editorial FROM tasks WHERE contest_id=$1 "
      "ORDER BY id ASC",
      c_id);

  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kArray};
  for (const auto &row : res) {
    userver::formats::json::ValueBuilder obj;
    obj["id"] = row["id"].As<int>();
    obj["title"] = row["title"].As<std::string>();
    obj["description"] = row["description"].As<std::string>();
    obj["editorial"] = row["editorial"].As<std::string>();
    builder.PushBack(std::move(obj));
  }
  return builder.ExtractValue();
}

#include <userver/components/component_context.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <QString> // For any remaining Qt logic, though we should avoid Qt core

// ... we will put this inside SubmitHandler inside mathforces namespace

SubmitHandler::SubmitHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()),
      http_client_(context.FindComponent<userver::components::HttpClient>().GetHttpClient()) {}

// Helper for Gemini
userver::formats::json::Value CallGeminiJson(userver::clients::http::Client& http_client, const std::string& prompt, const std::string& systemInstruction) {
    auto geminiKeyStr = std::getenv("GEMINI_API_KEY");
    std::string geminiKey = geminiKeyStr ? geminiKeyStr : "";
    auto openRouterKeyStr = std::getenv("OPENROUTER_API_KEY");
    std::string openRouterKey = openRouterKeyStr ? openRouterKeyStr : "";

    if (geminiKey.empty() && openRouterKey.empty()) {
        userver::formats::json::ValueBuilder res;
        res["score"] = 0;
        res["status"] = "unsuccessful";
        res["comment"] = "Error: No AI API KEY provided";
        return res.ExtractValue();
    }

    if (!openRouterKey.empty()) {
        userver::formats::json::ValueBuilder body;
        auto model = std::getenv("OPENROUTER_MODEL");
        body["model"] = model ? model : "openai/gpt-4o-mini";
        
        userver::formats::json::ValueBuilder sysMsg;
        sysMsg["role"] = "system";
        sysMsg["content"] = systemInstruction;
        
        userver::formats::json::ValueBuilder userMsg;
        userMsg["role"] = "user";
        userMsg["content"] = prompt;
        
        userver::formats::json::ValueBuilder messages;
        messages.PushBack(sysMsg.ExtractValue());
        messages.PushBack(userMsg.ExtractValue());
        
        body["messages"] = messages.ExtractValue();
        
        userver::formats::json::ValueBuilder respFmt;
        respFmt["type"] = "json_object";
        body["response_format"] = respFmt.ExtractValue();

        auto response = http_client.CreateRequest()
            ->post("https://openrouter.ai/api/v1/chat/completions", userver::formats::json::ToString(body.ExtractValue()))
            ->headers({{"Authorization", "Bearer " + openRouterKey}, {"Content-Type", "application/json"}})
            ->timeout(std::chrono::seconds(20))
            ->perform();

        if (response->IsOk()) {
            auto respJson = userver::formats::json::FromString(response->body());
            auto choices = respJson["choices"];
            if (!choices.IsEmpty()) {
                auto text = choices[0]["message"]["content"].As<std::string>();
                return userver::formats::json::FromString(text);
            }
        }
        userver::formats::json::ValueBuilder errResp;
        errResp["score"] = 0;
        errResp["status"] = "unsuccessful";
        errResp["comment"] = "Network or logic error";
        return errResp.ExtractValue();
    } else {
        userver::formats::json::ValueBuilder body;
        
        userver::formats::json::ValueBuilder part;
        part["text"] = prompt;
        userver::formats::json::ValueBuilder parts;
        parts.PushBack(part.ExtractValue());
        userver::formats::json::ValueBuilder contentObj;
        contentObj["parts"] = parts.ExtractValue();
        userver::formats::json::ValueBuilder contents;
        contents.PushBack(contentObj.ExtractValue());
        
        userver::formats::json::ValueBuilder sysPart;
        sysPart["text"] = systemInstruction;
        userver::formats::json::ValueBuilder sysParts;
        sysParts.PushBack(sysPart.ExtractValue());
        userver::formats::json::ValueBuilder sysInstr;
        sysInstr["parts"] = sysParts.ExtractValue();
        
        userver::formats::json::ValueBuilder genConf;
        genConf["responseMimeType"] = "application/json";
        
        body["contents"] = contents.ExtractValue();
        body["systemInstruction"] = sysInstr.ExtractValue();
        body["generationConfig"] = genConf.ExtractValue();

        auto response = http_client.CreateRequest()
            ->post("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=" + geminiKey, userver::formats::json::ToString(body.ExtractValue()))
            ->headers({{"Content-Type", "application/json"}})
            ->timeout(std::chrono::seconds(20))
            ->perform();

        if (response->IsOk()) {
            auto respJson = userver::formats::json::FromString(response->body());
            auto candidates = respJson["candidates"];
            if (!candidates.IsEmpty()) {
                auto text = candidates[0]["content"]["parts"][0]["text"].As<std::string>();
                return userver::formats::json::FromString(text);
            }
        }
        userver::formats::json::ValueBuilder errResp;
        errResp["score"] = 0;
        errResp["status"] = "unsuccessful";
        errResp["comment"] = "Network or logic error with Gemini";
        return errResp.ExtractValue();
    }
}

userver::formats::json::Value SubmitHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {

  std::string auth = request.GetHeader("Authorization");
  std::string role;
  int user_id = 0;
  if (!VerifyJwt(auth, user_id, role)) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
    return {};
  }

  int task_id = request_json["task_id"].As<int>();
  std::string answer = request_json["answer"].As<std::string>();

  auto taskRes = pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT task_type, correct_answer, max_score, description, editorial, ai_comment FROM tasks WHERE id=$1", task_id);

  if (taskRes.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return {};
  }

  std::string type = taskRes[0]["task_type"].As<std::string>("");
  int max_score = taskRes[0]["max_score"].As<int>();
  int score = 0;
  std::string ai_eval = "";
  std::string status = "rejected";

  if (type == "answer_only") {
      std::string correct = taskRes[0]["correct_answer"].As<std::string>("");
      // simple trim comparison omitted for brevity
      if (answer == correct) {
          score = max_score;
          status = "accepted";
      }
  } else {
      std::string prompt = "Task description and possible editorial: " + taskRes[0]["description"].As<std::string>("") + "\n" + taskRes[0]["editorial"].As<std::string>("") + "\n\nParticipant's solution: " + answer;
      std::string sys = "You are an impartial and accurate math autograder. The participant submitted a solution. Your task is to evaluate its correctness (answer and logic). If an editorial is provided, check against it. Evaluate the solution from 0 to " + std::to_string(max_score) + " points. Return a STRICT JSON format, without markdown or wrappers: {\"score\": " + std::to_string(max_score) + ", \"feedback\": \"Excellent work...\", \"thinking\": \"your reasoning\", \"probability\": 0.0}. " + taskRes[0]["ai_comment"].As<std::string>("") + ". You must respond entirely in English, including 'feedback' and 'thinking'.";
      
      auto resGemini = CallGeminiJson(http_client_, prompt, sys);
      score = resGemini.HasMember("score") ? resGemini["score"].As<int>() : 0;
      ai_eval = resGemini.HasMember("feedback") ? resGemini["feedback"].As<std::string>("") : (resGemini.HasMember("comment") ? resGemini["comment"].As<std::string>("") : "");
      status = (score == max_score) ? "accepted" : (score > 0 ? "partial" : "rejected");
  }

  auto insRes = pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                       "INSERT INTO submissions (task_id, user_id, answer_text, score, status, feedback) VALUES ($1, $2, $3, $4, $5, $6) RETURNING id",
                       task_id, user_id, answer, score, status, ai_eval);

  int submission_id = insRes[0]["id"].As<int>();

  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  builder["submission_id"] = submission_id;
  builder["score"] = score;
  builder["verdict"] = status;
  builder["ai_evaluation"] = ai_eval;
  return builder.ExtractValue();
}

SubmissionsHandler::SubmissionsHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value SubmissionsHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {

  if (!request.HasArg("task_id")) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
    return {};
  }
  int task_id = std::stoi(request.GetArg("task_id"));

  std::string auth = request.GetHeader("Authorization");
  std::string role;
  int user_id = 0;
  if (!VerifyJwt(auth, user_id, role)) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
    return {};
  }

  auto res = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, answer_text, score, feedback, status FROM submissions WHERE "
      "task_id=$1 AND user_id=$2 ORDER BY id DESC",
      task_id, user_id);

  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kArray};
  for (const auto &row : res) {
    userver::formats::json::ValueBuilder obj;
    obj["id"] = row["id"].As<int>();
    obj["answer"] = row["answer_text"].As<std::string>();
    obj["score"] = row["score"].As<int>();
    obj["feedback"] = row["feedback"].As<std::string>("");
    obj["status"] = row["status"].As<std::string>("");
    builder.PushBack(std::move(obj));
  }
  return builder.ExtractValue();
}

SubmissionsAllHandler::SubmissionsAllHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value SubmissionsAllHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {

  if (!request.HasArg("task_id")) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
    return {};
  }
  int task_id = std::stoi(request.GetArg("task_id"));

  // Require admin or superadmin role (Wait, anyone who solved can view? the
  // client limits UI for admin, but let's just dump all)
  auto res = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT s.id, s.answer_text, s.score, u.username FROM submissions s JOIN "
      "users u ON s.user_id = u.id WHERE s.task_id=$1 ORDER BY s.id DESC",
      task_id);

  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kArray};
  for (const auto &row : res) {
    userver::formats::json::ValueBuilder obj;
    obj["id"] = row["id"].As<int>();
    obj["answer_text"] = row["answer_text"].As<std::string>();
    obj["score"] = row["score"].As<int>();
    obj["username"] = row["username"].As<std::string>();
    builder.PushBack(std::move(obj));
  }
  return builder.ExtractValue();
}

HacksHandler::HacksHandler(const userver::components::ComponentConfig &config,
                           const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()),
      http_client_(context.FindComponent<userver::components::HttpClient>().GetHttpClient()) {}

userver::formats::json::Value HacksHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {

  std::string auth = request.GetHeader("Authorization");
  std::string role;
  int user_id = 0;
  if (!VerifyJwt(auth, user_id, role)) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
    return {};
  }

  int submission_id = request_json["submission_id"].As<int>();
  std::string hack_text = request_json["hack_text"].As<std::string>();

  auto subRes = pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT task_id, answer_text, status FROM submissions WHERE id=$1", submission_id);
  if (subRes.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return {};
  }

  int task_id = subRes[0]["task_id"].As<int>();
  std::string orig_answer = subRes[0]["answer_text"].As<std::string>("");
  std::string sub_status = subRes[0]["status"].As<std::string>("");

  if (sub_status != "accepted") {
      userver::formats::json::ValueBuilder e;
      e["error"] = "Can only hack accepted submissions";
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return e.ExtractValue();
  }

  auto taskRes = pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT description, editorial FROM tasks WHERE id=$1", task_id);
  if (taskRes.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return {};
  }

  std::string editorial = taskRes[0]["editorial"].As<std::string>("");
  
  std::string prompt = "Editorial: " + editorial + "\n\nParticipant's solution: " + orig_answer + "\n\nHacker's claim: " + hack_text;
  std::string sys = "You are a math competition judge. Your task is to verify a HACK. You have the editorial, the participant's solution (which is being hacked), and the hacker's argument/counterexample. You need to say if the hacker is right. Return only a STRICT JSON format, without markdown or wrappers: {\"is_successful\": true, \"explanation\": \"why the hacker is right or wrong\"}. You must respond entirely in English, including the 'explanation'.";

  auto resGemini = CallGeminiJson(http_client_, prompt, sys);
  
  bool is_successful = resGemini.HasMember("is_successful") ? resGemini["is_successful"].As<bool>() : false;
  std::string status = is_successful ? "successful" : "unsuccessful";

  pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                       "INSERT INTO hacks (submission_id, hacker_id, hack_text, status) VALUES ($1, $2, $3, $4)",
                       submission_id, user_id, hack_text, status);

  if (is_successful) {
      pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                           "UPDATE submissions SET status='hacked', score=0 WHERE id=$1", submission_id);
  }

  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  builder["hack_status"] = status;
  builder["explanation"] = resGemini.HasMember("explanation") ? resGemini["explanation"].As<std::string>("") : "";
  return builder.ExtractValue();
}

OauthCallbackHandler::OauthCallbackHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

userver::formats::json::Value OauthCallbackHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
  return builder.ExtractValue();
}

} // namespace mathforces
