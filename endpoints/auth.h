//
// Created by christian on 8/25/22.
//

#ifndef GRYPHHACKS_ENDPOINTS_AUTH_H_
#define GRYPHHACKS_ENDPOINTS_AUTH_H_

#include <pqxx/result>
#include "../lib/json.hpp"
#include "../lib/httplib.h"
#include "../util.h"

namespace auth {

using namespace httplib;
using json = nlohmann::json;

auto login(const Request &req, Response &res) {
  try {
    json j = json::parse(req.body);
    pqxx::result r = util::get_from_sql(
        "SELECT salt, id, passwd FROM users WHERE email=\'"
            + j["email"].get<std::string>()
            + "\'"
    );
    auto id = r[0][1].as<std::string>(),
        passwd_hash = r[0][2].as<std::string>(),
        salt = r[0][0].as<std::string>();
    if (!util::valid_pass(passwd_hash, j["password"].get<std::string>(), salt)) {
      throw std::exception();
    }
    std::stringstream ss;
    ss << "INSERT INTO sessions (user_id) VALUES (" << id << ");";
    util::send_to_sql(ss.str());
    res.set_header("Set-Cookie",
                   "session="
                       + util::get_from_sql("SELECT uuid FROM sessions WHERE user_id=" + id)[0][0].as<std::string>());
    res.set_header("no", "thank you");

  } catch (const std::exception &e) {
    res.set_content("no", "text/plain");
    return;
  }
}

auto signup(const Request &req, Response &res) {
  try {
    json j = json::parse(req.body);
    // validate email with twilio
    uint64_t salt = util::gen_snowflake(1234) >> 1;
    std::stringstream ss;
    ss << "INSERT INTO users (id, f_name, l_name, email, passwd, salt, is_host)"
       << "VALUES ("
       << (util::gen_snowflake(214) >> 1) << ",'"
       << j["f_name"].get<std::string>() << "','"
       << j["l_name"].get<std::string>() << "','"
       << j["email"].get<std::string>() << "','"
       << util::sha256(j["password"].get<std::string>() + std::to_string(salt)) << "',"
       << salt << ","
       << (j["is_host"].get<bool>() ? "true" : "false") << ")";
    util::send_to_sql(ss.str());
  } catch (const std::exception &e) {
    res.set_content("no", "text/plain");
    return;
  }
}

}

#endif //GRYPHHACKS_ENDPOINTS_AUTH_H_
