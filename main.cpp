#include <iostream>
#include <string>
#include <pqxx/pqxx>
#include <cstdio>
#include <uWebSockets/App.h>
#include <jwt.h>
//#include <gen_token-cpp/traits/nlohmann-json/traits.h>
#include <jwt-cpp/traits/kazuho-picojson/traits.h>
#include "lib/json.hpp"
#include "lib/picosha2.h"
#include "util.h"
#include "settings.h"
#include "endpoints/misc.h"
#include "endpoints/user.h"
#include "endpoints/event.h"
#include "endpoints/auth.h"

/*
 * TODO:
 * POST REQUESTS:
 * update_profile
 * update_event
 * org_create
 */

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

using namespace std::chrono_literals;
int main(int argc, char **argv) {

  auto ip = get_local_ip();
  if (argc > 1) {
    ip = argv[1];
  }
  settings::sql_url = parse_url();
  get_local_ip();

  uWS::App svr;

  // region api requests


  svr.get("/api/ping", misc::ping);

  svr.get(R"(/api/usr/:user_id)", user::user_get);

  svr.post("/api/events/create", event::event_create);
  svr.get("/api/events/all", event::event_get_all);
  svr.get("/api/events/loc", event::event_get_loc);
  svr.get(R"(/api/events/:event_id)", event::event_get);
  svr.get(R"(/api/events/:event_id/rsvp)", event::event_get_rsvp);
  svr.post(R"(/api/events/:event_id/rsvp)", event::event_set_rsvp);

  svr.post("/api/login", auth::login);
  svr.post("/api/signup", auth::signup);

  //endregion

  std::cout << "IP: " << ip << std::endl;
  svr.listen(ip, 8080, [](auto *listenSocket) {
    if (listenSocket) {
      std::cout << "Serving content" << std::endl;
    }
  });
  svr.run();
  return 0;
}
