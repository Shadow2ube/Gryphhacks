//#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <iostream>
#include <string>
#include <pqxx/pqxx>
#include <cstdio>
#include "lib/httplib.h"
#include "lib/json.hpp"
#include "util.h"
#include "lib/picosha2.h"

#define RES [&](const httplib::Request &req, httplib::Response &res)
#define REQ [&](const httplib::Request &req, httplib::Response &)

/*
 * TODO:
 * POST REQUESTS:
 * update_profile
 * update_event
 * org_create
 */

using httplib::Server;
using nlohmann::json;
using namespace std::chrono;
using namespace util;

std::string parse_url() {
  std::ifstream in;
  in.open("./settings.no");
  std::string line;
  getline(in, line);
  return line;
}
bool valid_pass(const std::string &hash, const std::string &passwd, const std::string &salt);

enum perm_level {
  NO_AUTH = 0,
  USER = 1,
  HOST = 2,
  ADMIN = 3
};

perm_level get_perms(const std::string &url, const std::string &session_id) {
  try {
    pqxx::result r = get_from_sql(url,
                                  "SELECT users.is_admin, users.is_host, sessions.uuid FROM users "
                                  "INNER JOIN sessions ON sessions.user_id=users.id "
                                  "WHERE sessions.uuid='" + session_id + "\'");
    bool is_admin = r[0][0].as<bool>(), is_host = r[0][1].as<bool>();
    if (is_admin) return perm_level::ADMIN;
    if (is_host) return perm_level::HOST;
  } catch (std::exception &ignored) {
    return perm_level::NO_AUTH;
  }
  return perm_level::USER;
}

int main() {
  std::string sql_url = parse_url();
  get_local_ip();

  Server svr;

  // region api requests

  svr.Get("/api/ping", [](const auto &req, auto &res) {
    res.set_content("pong", "text/plain");
  });

  svr.Get(R"(/api/usr/(\d+))", RES {
    auto id = req.matches[1];
    pqxx::result r = get_from_sql(sql_url, "SELECT id, f_name, l_name, is_host from users where id=" + std::string
        (id));

    json j = {
        {"id", r[0][0].as<uint64_t>()},
        {"f_name", r[0][1].as<std::string>()},
        {"l_name", r[0][2].as<std::string>()},
        {"is_host", r[0][3].as<bool>()},
    };
    res.set_content(j.dump(), "application/json");
  });

  svr.Get("/api/events/loc", RES {
    pqxx::result r = get_from_sql(sql_url, "SELECT id, name, location, lat, long FROM events");

    json j = json::array();
    for (auto row: r) {
      j += {
          {"id", row[0].as<std::string>()},
          {"name", row[1].as<std::string>()},
          {"location", row[2].as<std::string>()},
          {"lat", row[3].as<std::string>()},
          {"long", row[4].as<std::string>()},
      };
    }
//    res.set_content(j.dump(), "application/json");
    res.set_content("no", "text/plain");
//    res.set_content("yeet", "text/plain");
  });

  svr.Get("/api/events/all", RES {
    try {
      pqxx::result r = get_from_sql(
          sql_url,
          "SELECT id, user_id, min_age, is_recurring, name, description, long, lat, "
          "time_start, time_end, location, email "
          "FROM events");

      json j = json::array();
      for (auto row: r) {
        j += {
            {"id", row[0].as<std::string>()},
            {"user_id", row[1].as<std::string>()},
            {"min_age", row[2].as<std::string>()},
            {"is_recurring", row[3].as<std::string>()},
            {"name", row[4].as<std::string>()},
            {"description", row[5].as<std::string>()},
            {"long", row[6].as<std::string>()},
            {"lat", row[7].as<std::string>()},
            {"time_start", row[8].as<std::string>()},
            {"time_end", row[9].as<std::string>()},
            {"location", row[10].as<std::string>()},
            {"email", row[11].as<std::string>()},
        };
      }
      res.set_content(j.dump(), "application/json");
    } catch (std::exception &e) {
      res.set_content("no", "text/plain");
    }
  });

  svr.Post("/api/login", [sql_url](
      const httplib::Request &req,
      httplib::Response &res
  ) {
    try {
      json j = json::parse(req.body);
      pqxx::result r = get_from_sql(
          sql_url,
          "SELECT salt, id, passwd FROM users WHERE email=\'"
              + j["email"].get<std::string>()
              + "\'"
      );
      auto id = r[0][1].as<std::string>(),
          passwd_hash = r[0][2].as<std::string>(),
          salt = r[0][0].as<std::string>();
      if (!valid_pass(passwd_hash, j["password"].get<std::string>(), salt)) {
        throw std::exception();
      }
      std::stringstream ss;
      ss << "INSERT INTO sessions (user_id) VALUES (" << id << ");";
      send_to_sql(sql_url, ss.str());
      res.set_header("Cookie",
                     "session=" + get_from_sql(
                         sql_url, "SELECT uuid FROM sessions WHERE user_id="
                             + id)[0][0].as<std::string>());
      res.set_header("no", "thank you");

    } catch (const std::exception &e) {
      res.set_content("no", "text/plain");
      return;
    }
  });

  svr.Post("/api/events/create", [sql_url](
      const httplib::Request &req,
      httplib::Response &res
  ) {
    json j = json::parse(req.body);
    try {
      std::stringstream ss;
      ss << "INSERT INTO events (id, user_id, min_age, name, description, lat, long, time_start, time_end, "
            "location, email, is_recurring) VALUES ("
         << (gen_snowflake(10) >> 1) << ","
         // << uid_from_session(sql_url, req.get_header_value("auth")) << ","
         << (gen_snowflake(6464) >> 1) << ","
         << j["min_age"].get<int>() << ",'"
         << j["name"].get<std::string>() << "','"
         << j["description"].get<std::string>() << "',"
         << j["lat"].get<float>() << ","
         << j["long"].get<float>() << ", TIMESTAMP '"
         << j["time_start"].get<std::string>() << "', TIMESTAMP '"
         << j["time_end"].get<std::string>() << "','"
         << j["location"].get<std::string>() << "', '"
         << j["email"].get<std::string>() << "', "
         << (j["is_recurring"].get<bool>() ? "true" : "false")
         << ")";
      send_to_sql(sql_url, ss.str());
    } catch (const std::exception &e) {
      res.set_content("no", "text/plain");
      return;
    }
  });

  svr.Post("/api/signup", [sql_url](
      const httplib::Request &req,
      httplib::Response &res
  ) {
    try {
      json j = json::parse(req.body);
      // validate email with twilio
      uint64_t salt = gen_snowflake(1234) >> 1;
      std::stringstream ss;
      ss << "INSERT INTO users (id, f_name, l_name, email, passwd, salt, is_host)"
         << "VALUES ("
         << (gen_snowflake(214) >> 1) << ",'"
         << j["f_name"].get<std::string>() << "','"
         << j["l_name"].get<std::string>() << "','"
         << j["email"].get<std::string>() << "','"
         << sha256(j["password"].get<std::string>() + std::to_string(salt)) << "',"
         << salt << ","
         << (j["is_host"].get<bool>() ? "true" : "false") << ")";
      send_to_sql(sql_url, ss.str());
    } catch (const std::exception &e) {
      res.set_content("no", "text/plain");
      return;
    }
  });

  svr.Options("/api/login", [](const auto &req, auto &res) {
    res.set_header("Access-Control-Allow-Origin", "*");
  });

  //endregion

  svr.set_logger([](const auto &req, const auto &res) {
    std::cout << "req: " << req.body << "\t-\tres: " << res.body << std::endl;
  });
  svr.set_post_routing_handler([](const auto &req, auto &res) {
    res.set_header("Access-Control-Allow-Origin", "*");
  });
  svr.set_read_timeout(5, 0); // 5 seconds
  svr.set_write_timeout(5, 0); // 5 seconds
  svr.listen(get_local_ip(), 8080);
  return 0;
}

bool valid_pass(const std::string &hash, const std::string &passwd, const std::string &salt) {
  return hash == sha256(passwd + salt);
}