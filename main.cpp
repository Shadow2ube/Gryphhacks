#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <iostream>
#include <string>
#include <pqxx/pqxx>
#include <cstdio>
#include <yaml-cpp/yaml.h>
#include <yaml.h>
#include "httplib.h"
#include "json.hpp"
#include "util.h"
#include "picosha2.h"

#define RES [&](const httplib::Request &req, httplib::Response &res)
#define REQ [&](const httplib::Request &req, httplib::Response &)

/*
 * TODO:
 * POST REQUESTS:
 * login
 * add_event
 * update_profile
 * update_event
 * user_create
 * org_create
 * search
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

  Server svr;  //("./ignore/cert.pem", "./ignore/key.pem");
  gen_snowflake(22);

  // region api requests

  svr.Get(R"(/api/usr/(\d+))", RES {
    auto id = req.matches[1];
    pqxx::result r = get_from_sql(sql_url, "SELECT id, f_name, l_name, is_host from users where id=" + std::string
        (id));

    json j = {
        {"id", r[0][0].as<int>()},
        {"f_name", r[0][1].as<std::string>()},
        {"l_name", r[0][2].as<std::string>()},
        {"is_host", r[0][3].as<bool>()},
    };
    res.set_content(j.dump(), "application/json");
  });

  svr.Get("/api/events/loc", RES {
    pqxx::result r = get_from_sql(sql_url, "SELECT id, name, lat, long FROM events");

    json j = json::array();
    for (auto row: r) {
      j += {
          {"id", row[0].as<int>()},
          {"name", row[1].as<std::string>()},
          {"location", row[2].as<std::string>()},
          {"lat", row[3].as<float>()},
          {"long", row[4].as<float>()},
      };
    }
    res.set_content(j.dump(), "application/json");
//    res.set_content("yeet", "text/plain");
  });

  svr.Get("/api/events/all", RES {
    pqxx::result r = get_from_sql(sql_url, "SELECT id, user_id, min_age, is_recurring, name, description, long, "
                                           "lat, "
                                           "time_start, time_end, location FROM events");

    json j = json::array();
    for (auto row: r) {
      j += {
          {"id", row[0].as<int>()},
          {"user_id", row[1].as<int>()},
          {"min_age", row[2].as<int>()},
          {"is_recurring", row[3].as<bool>()},
          {"name", row[4].as<std::string>()},
          {"description", row[5].as<std::string>()},
          {"long", row[6].as<float>()},
          {"lat", row[7].as<float>()},
          {"time_start", row[8].as<std::string>()},
          {"time_end", row[9].as<std::string>()},
          {"location", row[10].as<std::string>()},
      };
    }
    res.set_content(j.dump(), "application/json");
  });

  svr.Post("/api/login", [sql_url](
      const httplib::Request &req,
      httplib::Response &res,
      const httplib::ContentReader &content_reader
  ) {
    if (req.is_multipart_form_data()) {
      json j = read_multipart_form(req, res, content_reader);
      try {
        pqxx::result r = get_from_sql(
            sql_url,
            "SELECT salt, id, passwd FROM users WHERE email=\'"
                + remove_of(j["email"]["content"])
                + "\'"
        );
        auto id = r[0][1].as<std::string>(),
            passwd_hash = r[0][2].as<std::string>(),
            salt = r[0][0].as<std::string>();
        if (!valid_pass(passwd_hash, j["password"]["content"].get<std::string>(), salt)) {
          throw std::exception();
        }
        std::stringstream ss;
        std::cout << "a" << std::endl;
        ss << "INSERT INTO sessions (user_id) VALUES (" << id << ");";
        std::cout << "b" << std::endl;
        send_to_sql(sql_url, ss.str());
        std::cout << "c" << std::endl;
        std::cout << "d" << std::endl;
        res.set_header("Cookie",
                       "session="
                           + get_from_sql(sql_url,
                                          "SELECT uuid FROM sessions WHERE user_id=" + id)[0][0].as<std::string>()
        );
        std::cout << "e" << std::endl;
      } catch (const std::exception &e) {
        res.set_content("INVALID EMAIL OR PASSWORD, err: " + std::string(e.what()), "text/plain");
        return;
      }
    } else {
      res.set_content("INVALID FORM INPUT", "text/plain");
    }
  });

  svr.Post("/api/events/create", [sql_url](
      const httplib::Request &req,
      httplib::Response &res,
      const httplib::ContentReader &content_reader
  ) {
    if (get_perms(sql_url, req.get_header_value("Cookie").substr(5)) <= 1) {
      res.set_content("Unauthorized", "text/plain");
      return;
    }
    if (req.is_multipart_form_data()) {
      json j = read_multipart_form(req, res, content_reader);
      try {
        std::stringstream ss;
        ss << "INSERT INTO events (id, user_id, min_age, name, description, lat, long, time_start, time_end, "
              "location) VALUES (" << (gen_snowflake(10) >> 1) << ","
           //              << uid_from_session(sql_url, req.get_header_value("auth")) << ","
           << "1234" << ","
           << std::stoi(j["min_age"]["content"].get<std::string>()) << ",'"
           << j["name"]["content"].get<std::string>() << "','"
           << j["description"]["content"].get<std::string>() << "',"
           << std::stof(j["lat"]["content"].get<std::string>()) << ","
           << std::stof(j["long"]["content"].get<std::string>()) << ", TIMESTAMP '"
           << j["time_start"]["content"].get<std::string>() << "', TIMESTAMP '"
           << j["time_end"]["content"].get<std::string>() << "','"
           << j["location"]["content"].get<std::string>() << "')";
        send_to_sql(sql_url, ss.str());
      } catch (const std::exception &e) {
        res.set_content("invalid: " + std::string(e.what()), "text/plain");
        return;
      }
    } else {
      std::string body;
      content_reader([&](const char *data, size_t data_length) {
        body.append(data, data_length);
        return true;
      });
      res.set_content("INVALID FORM INPUT: " + body, "text/plain");
    }
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


