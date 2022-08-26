//#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <iostream>
#include <string>
#include <pqxx/pqxx>
#include <cstdio>
#include "lib/httplib.h"
#include "lib/json.hpp"
#include "lib/picosha2.h"
#include "util.h"
#include "settings.h"
#include "endpoints/misc.h"
#include "endpoints/user.h"
#include "endpoints/event.h"
#include "endpoints/auth.h"

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

  svr.Get("/api/ping", misc::ping);

  svr.Get(R"(/api/usr/(\d+))", user::user_get);

  svr.Post("/api/events/create", event::event_create);
  svr.Get("/api/events/all", event::event_get_all);
  svr.Get("/api/events/loc", event::event_get_loc);
  svr.Get(R"(/api/events/(\d+))", event::event_get);
  svr.Get(R"(/api/events/(\d+)/rsvp)", event::event_get_rsvp);
  svr.Post(R"(/api/events/(\d+)/rsvp)", event::event_set_rsvp);

  svr.Post("/api/login", auth::login);
  svr.Post("/api/signup", auth::signup);

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
