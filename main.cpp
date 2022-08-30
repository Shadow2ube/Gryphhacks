#include <iostream>
#include <string>
#include <pqxx/pqxx>
#include <cstdio>
#include <uWebSockets/App.h>
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

//typedef websocketpp::server<websocketpp::config::asio> server;
//using websocketpp::lib::placeholders::_1;
//using websocketpp::lib::placeholders::_2;
//using websocketpp::lib::bind;
//
//typedef server::message_ptr message_ptr;

//class echo_handler : public server::handler {
//  void on_message(connection_ptr con, std::string msg) {
//    con->write(msg);
//  }
//};

//void on_message(server *s, websocketpp::connection_hdl hdl, message_ptr msg) {
//  std::cout << "on_message called with hdl: " << hdl.lock().get()
//            << " and message: " << msg->get_payload() << std::endl;
//
//  if (msg->get_payload() == "stop-listening") {
//    s->stop_listening();
//    return;
//  }
//
//  try {
//    s->send(hdl, msg->get_payload(), msg->get_opcode());
//  } catch (websocketpp::exception const &e) {
//    std::cout << "Echo failed because: " << "(" << e.what() << ")" << std::endl;
//  }
//}

int main(int argc, char **argv) {
  auto ip = get_local_ip();
  if (argc > 1) {
    ip = argv[1];
  }
  settings::sql_url = parse_url();
  get_local_ip();

//  Server svr;
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


//  svr.set_logger([](const auto &req, const auto &res) {
//    std::cout << "req: " << req.body << "\t-\tres: " << res.body << std::endl;
//  });
//  svr.set_read_timeout(5, 0); // 5 seconds
//  svr.set_write_timeout(5, 0); // 5 seconds
  std::cout << "IP: " << ip << std::endl;
//  svr.listen(ip, 8080);
  svr.listen(ip, 8080, [](auto *listenSocket) {
    if (listenSocket) {
      std::cout << "Serving content" << std::endl;
    }
  });
  svr.run();
  return 0;
}
