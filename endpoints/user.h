//
// Created by christian on 8/21/22.
//

#ifndef GRYPHHACKS_ENDPOINTS_USER_H_
#define GRYPHHACKS_ENDPOINTS_USER_H_

#include <pqxx/result>
#include "../settings.h"

namespace user {

typedef uWS::HttpResponse<false> Response;
typedef uWS::HttpRequest Request;

auto user_get(Response *res, Request *req) {
  std::string id{req->getParameter(0)};
  pqxx::result r = util::get_from_sql(
      "SELECT id, f_name, l_name, is_host from users where id=" + id);

  json j = {
      {"id", r[0][0].as<uint64_t>()},
      {"f_name", r[0][1].as<std::string>()},
      {"l_name", r[0][2].as<std::string>()},
      {"is_host", r[0][3].as<bool>()},
  };
  res->end(j.dump());
}

}

#endif //GRYPHHACKS_ENDPOINTS_USER_H_
