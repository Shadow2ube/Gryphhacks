//#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <iostream>
#include <string>
#include <pqxx/pqxx>
#include <cstdio>
#include "lib/httplib.h"
#include "lib/json.hpp"
#include "util.h"
#include "lib/picosha2.h"
#include "endpoints/misc.h"
#include "endpoints/user.h"
#include "settings.h"

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
  in.open("/etc/sevahub/settings.no");
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

int main(int argc, char **argv) {
  auto ip = get_local_ip();
  if (argc > 1) {
    ip = argv[1];
  }
  settings::sql_url = parse_url();
  get_local_ip();

  Server svr;

  // region api requests

  svr.Get("/api/ping", [](const auto &req, auto &res) {
    res.set_content("pong", "text/plain");
  });

  svr.Get(R"(/api/usr/(\d+))", RES {
    std::string id = req.matches[1];
    pqxx::result r = get_from_sql(
        "SELECT id, f_name, l_name, is_host from users where id=" + id);

    json j = {
        {"id", r[0][0].as<uint64_t>()},
        {"f_name", r[0][1].as<std::string>()},
        {"l_name", r[0][2].as<std::string>()},
        {"is_host", r[0][3].as<bool>()},
    };
    res.set_content(j.dump(), "application/json");
  });

  svr.Get("/api/events/loc", RES {
    pqxx::result r = get_from_sql("SELECT id, name, location, lat, long FROM events");

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
    res.set_content(j.dump(), "application/json");
//    res.set_content("no", "text/plain");
//    res.set_content("yeet", "text/plain");
  });

  svr.Get("/api/events/all", RES {
    try {
      pqxx::result r = get_from_sql(
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

  svr.Get(R"(/api/events/(\d+))", [](const auto &req, auto &res) {
    try {
      std::string id = req.matches[1];
      auto cookies = util::get_cookies(req.get_header_value("Cookie"));
      auto r = get_from_sql("SELECT id, user_id, min_age, is_recurring, name, description, long, lat, "
                            "time_start, time_end, location, email "
                            "FROM events WHERE id='" + id + "'");
      json j = {
          {"id", r[0][0].as<std::string>()},
          {"user_id", r[0][1].as<std::string>()},
          {"min_age", r[0][2].as<std::string>()},
          {"is_recurring", r[0][3].as<std::string>()},
          {"name", r[0][4].as<std::string>()},
          {"description", r[0][5].as<std::string>()},
          {"long", r[0][6].as<std::string>()},
          {"lat", r[0][7].as<std::string>()},
          {"time_start", r[0][8].as<std::string>()},
          {"time_end", r[0][9].as<std::string>()},
          {"location", r[0][10].as<std::string>()},
          {"email", r[0][11].as<std::string>()},
      };
      res.set_content(j.dump(), "application/json");
    } catch (std::exception &e) {
      res.set_content("no", "text/plain");
    }
  });

  svr.Get(R"(/api/events/(\d+)/rsvp)", [](const auto &req, auto &res) {
    try {
      std::string id = req.matches[1];
      auto cookies = util::get_cookies(req.get_header_value("Cookie"));
      pqxx::result r = get_from_sql("SELECT user_id FROM event_rsvp WHERE event_id='"
                                        + id
                                        + "'");
      json j;
      for (auto i: r) {
        j["rsvp"].push_back(i[0].as<uint64_t>());
      }
      res.set_content(j.dump(), "application/json");
    } catch (std::exception &e) {
      res.set_content("no", "text/plain");
    }
  });

  svr.Post(R"(/api/events/(\d+)/rsvp)", [](const auto &req, auto &res) {
    try {
      auto cookies = util::get_cookies(req.get_header_value("Cookie"));
      json body = json::parse(req.body);
      std::string event_id = req.matches[1];
      uint64_t user_id = uid_from_session(cookies["session"]);
      if (body["rsvp"]) {
        pqxx::result r = get_from_sql("SELECT COUNT(1) FROM event_rsvp WHERE user_id='"
                                          + std::to_string(user_id)
                                          + "' AND event_id='"
                                          + event_id
                                          + "'");
        if (!r[0][0].as<bool>()) {
          send_to_sql("INSERT INTO event_rsvp (event_id, user_id) VALUES ("
                          + event_id + ","
                          + std::to_string(user_id) + ")");
        }
      } else {
        send_to_sql("DELETE FROM event_rsvp WHERE user_id='"
                        + std::to_string(user_id)
                        + "'");
      }
    } catch (std::exception &e) {
      res.set_content("no", "text/plain");
    }
  });

  svr.Post("/api/login", [](
      const httplib::Request &req,
      httplib::Response &res
  ) {
    try {
      json j = json::parse(req.body);
      pqxx::result r = get_from_sql(
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
      send_to_sql(ss.str());
      res.set_header("Set-Cookie",
                     "session="
                         + get_from_sql("SELECT uuid FROM sessions WHERE user_id=" + id)[0][0].as<std::string>());
      res.set_header("no", "thank you");

    } catch (const std::exception &e) {
      res.set_content("no", "text/plain");
      return;
    }
  });

  svr.Post("/api/events/create", [](
      const httplib::Request &req,
      httplib::Response &res
  ) {
    json j = json::parse(req.body);
    auto cookies = util::get_cookies(req.get_header_value("Cookie"));
    try {
      auto event_id = gen_snowflake(10) >> 1;
      std::stringstream ss;
      ss << "INSERT INTO events (id, user_id, min_age, name, description, lat, long, time_start, time_end, "
            "location, email, is_recurring) VALUES ("
         << event_id << ","
         << uid_from_session(cookies["session"]) << ","
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
      send_to_sql(ss.str());
      res.set_content(R"({"id":")" + std::to_string(event_id) + "\"}", "application/json");
    } catch (const std::exception &e) {
      std::cout << e.what() << std::endl;
      res.set_content("no", "text/plain");
      return;
    }
  });

  svr.Post("/api/signup", [](
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
      send_to_sql(ss.str());
    } catch (const std::exception &e) {
      res.set_content("no", "text/plain");
      return;
    }
  });

  //endregion

  svr.set_logger([](const auto &req, const auto &res) {
    std::cout << "req: " << req.body << "\t-\tres: " << res.body << std::endl;
  });
  svr.set_read_timeout(5, 0); // 5 seconds
  svr.set_write_timeout(5, 0); // 5 seconds
  std::cout << "IP: " << ip << std::endl;
  svr.listen(ip, 8080);
  return 0;
}

bool valid_pass(const std::string &hash, const std::string &passwd, const std::string &salt) {
  return hash == sha256(passwd + salt);
}