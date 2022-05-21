//#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <iostream>
#include <string>
#include <chrono>
#include "httplib.h"
#include "json.hpp"
#include <pqxx/pqxx>
#include <chrono>

#define RES [](const httplib::Request &req, httplib::Response &res)
#define REQ [](const httplib::Request &req, httplib::Response &)
#define SQL_URL "postgresql://hecker:HIh05t8y3KT7LJC7EtDmag@free-tier11.gcp-us-east1.cockroachlabs" \
".cloud:26257/test?sslmode=verify-full&options=--cluster%3Dchill-whale-766"
#define SQL_CONN pqxx::connection CONN(SQL_URL); pqxx::nontransaction conn(CONN)

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

int64_t snowflake_gen(int wid, int pid, int seq) {
//  int t = std::chrono::duration_cast<std::chrono::milliseconds>((_t - ).count();
  return 1;
}

pqxx::result get_from_sql(const std::string &s) {
  // unsafe lol
//  try {
  SQL_CONN;
  return conn.exec(s);
//  } catch (std::exception &e) {
//    std::cerr << e.what() << std::endl;
//  }
}

int main() {
  Server svr;  //("./ignore/cert.pem", "./ignore/key.pem");
  snowflake_gen(1, 2, 3);

  svr.Get(R"(/api/usr/(\d+))", RES {
    auto id = req.matches[1];
    pqxx::result r = get_from_sql("SELECT id, f_name, l_name, is_host from users where id=" + std::string(id));

    json j = json::object();
    j += {
        {"id", r[0][0].as<int>()},
        {"f_name", r[0][1].as<std::string>()},
        {"l_name", r[0][2].as<std::string>()},
        {"is_host", r[0][3].as<bool>()},
    };
    res.set_content(j.dump(), "application/json");
  });

  svr.Get("/api/events/loc", RES {
    pqxx::result r = get_from_sql("SELECT id, name, lat, long FROM events");

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
    pqxx::result r = get_from_sql("SELECT id, user_id, min_age, is_recurring, name, description, long, lat, "
                                  "time_start, time_end FROM events");

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
      };
    }
    res.set_content(j.dump(), "application/json");
  });

  svr.Post("/api/login", RES {
    res.set_header("a_token_4_u", "adfojlnferjkedrr3qiojur3oiuj3roijoji3fnlukwveoujihsjveokwlnvwrejlkn");
  });

  svr.set_logger([](const auto &req, const auto &res) {
    std::cout << "req: " << req.body << "\t-\tres: " << res.body << std::endl;
  });
  svr.set_read_timeout(5, 0); // 5 seconds
  svr.set_write_timeout(5, 0); // 5 seconds
  svr.listen("127.0.0.1", 8080);
  return 0;
}
