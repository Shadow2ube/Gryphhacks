//
// Created by christian on 8/21/22.
//

#ifndef GRYPHHACKS_ENDPOINTS_MISC_H_
#define GRYPHHACKS_ENDPOINTS_MISC_H_

#include "../lib/httplib.h"

using namespace httplib;

namespace misc {

auto ping(const Request &req, Response &res) {
  res.set_content(R"({"ping": "pong"})", "application/json");
}

}

#endif //GRYPHHACKS_ENDPOINTS_MISC_H_
