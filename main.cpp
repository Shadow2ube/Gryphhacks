// #define CPPHTTPLIB_OPENSSL_SUPPORT

#include <iostream>
#include <string>
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
 * POST REQUESTS:
 * login
 * add_event
 * update_profile
 * update_event
 * user_create
 * org_create
 * search
 *
 * GET REQUESTS:
 * /usr/(user_id)
 */
using httplib::Server;
using nlohmann::json;
using namespace std::chrono;

int64_t snowflake_gen(int wid, int pid, int seq) {

  milliseconds ms = duration_cast<milliseconds>(
      system_clock::now().time_since_epoch()
  );
//  std::chrono::duration<long> dur(duration);

  int t = time(nullptr);
  std::cout << t << std::endl;
  for (int i = 31; i >= 0; --i) {
    std::cout << ((t >> i) & 1);
  }
  std::cout << std::endl;
}

pqxx::result get_from_sql(const std::string &s) {
  try {
    SQL_CONN;
    return conn.exec(s);
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}

int main() {
  Server svr;
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
          {"lat", row[2].as<float>()},
          {"long", row[3].as<float>()},
      };
    }
    res.set_content(j.dump(), "application/json");
  });

  svr.listen("127.0.0.1", 8080);

  std::cout << "omg i wrote code" << std::endl;
  return 0;
}
