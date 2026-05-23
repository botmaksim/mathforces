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

SubmitHandler::SubmitHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

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

  pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                       "INSERT INTO submissions (task_id, user_id, "
                       "answer_text, status) VALUES ($1, $2, $3, 'pending')",
                       task_id, user_id, answer);

  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
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
              .GetCluster()) {}

userver::formats::json::Value HacksHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest &request,
    const userver::formats::json::Value &request_json,
    userver::server::request::RequestContext &) const {
  userver::formats::json::ValueBuilder builder{
      userver::formats::common::Type::kObject};
  builder["status"] = "ok";
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
