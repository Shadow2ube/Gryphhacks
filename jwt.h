//
// Created by christian on 8/30/22.
//

#ifndef GRYPHHACKS__JWT_H_
#define GRYPHHACKS__JWT_H_

#include "lib/json.hpp"

using json = nlohmann::json;

struct jwt {
    bool is_admin;

 private:
  std::string sig = "HS256"

};

#endif //GRYPHHACKS__JWT_H_
