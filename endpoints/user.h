//
// Created by christian on 8/21/22.
//

#ifndef GRYPHHACKS_ENDPOINTS_USER_H_
#define GRYPHHACKS_ENDPOINTS_USER_H_

#include <pqxx/result>
#include "../lib/httplib.h"
#include "../settings.h"

using namespace httplib;

namespace user {

auto user_get(const Request &req, Response &res) {
  std::string id = req.matches[1];
  pqxx::result r = util::get_from_sql(
      "SELECT id, f_name, l_name, is_host from users where id=" + id);

  json j = {
      {"id", r[0][0].as<uint64_t>()},
      {"f_name", r[0][1].as<std::string>()},
      {"l_name", r[0][2].as<std::string>()},
      {"is_host", r[0][3].as<bool>()},
  };
  res.set_content(j.dump(), "application/json");
}

}

#endif //GRYPHHACKS_ENDPOINTS_USER_H_
