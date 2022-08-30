//
// Created by christian on 8/21/22.
//

#ifndef GRYPHHACKS_ENDPOINTS_MISC_H_
#define GRYPHHACKS_ENDPOINTS_MISC_H_

#include <uWebSockets/App.h>

namespace misc {

using namespace uWS;
typedef uWS::HttpResponse<false> Response;
typedef uWS::HttpRequest Request;

auto ping(Response *res, Request *req) {
  res->end(R"({"ping": "pong"})");
}

}

#endif //GRYPHHACKS_ENDPOINTS_MISC_H_
