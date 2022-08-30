//
// Created by christian on 8/25/22.
//

#ifndef GRYPHHACKS_ENDPOINTS_AUTH_H_
#define GRYPHHACKS_ENDPOINTS_AUTH_H_

#include <pqxx/result>
#include "../lib/json.hpp"
#include "../util.h"

namespace auth {

using json = nlohmann::json;

int x = 0;
std::string letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
auto test() {
  if (x >= letters.size()) x = 0;
  std::cout << letters[x++] << std::endl;
}

auto login(Response *res, Request *) {
  util::handle(res, [res](auto body) -> std::string {
    try {
      test();
      json j = json::parse(body);
      test();
      pqxx::result r =
          util::get_from_sql("SELECT salt, id, passwd FROM users WHERE email=\'" + j["email"].get<std::string>() + "\'"
          );
      auto id = r[0][1].as<std::string>();
      auto passwd_hash = r[0][2].as<std::string>();
      auto salt = r[0][0].as<std::string>();
      if (!util::valid_pass(passwd_hash, j["password"].get<std::string>(), salt)) {
        throw std::exception();
      }
      std::stringstream ss;
      ss << "INSERT INTO sessions (user_id) VALUES (" << id << ");";
      util::send_to_sql(ss.str());
      res->writeHeader("Set-Cookie",
                       "session="
                           + util::get_from_sql(
                               "SELECT uuid FROM sessions WHERE user_id=" + id)[0][0].as<std::string>());

      return "{}";
    } catch (const std::exception &e) {
      std::cout << "An error occurred" << std::endl;
      return R"({"value": "invalid credentials"})";
    }
  });
}

auto signup(Response *res, Request *req) {
  util::handle(res, [](auto body) {
    try {
      json j = json::parse(body);
//     validate email with twilio
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
      return "";
    } catch (const std::exception &e) {
      return R"({"value": "server error"})";
    }
  });
}

}

#endif //GRYPHHACKS_ENDPOINTS_AUTH_H_
