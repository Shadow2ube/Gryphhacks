//
// Created by christian on 5/21/22.
//

#ifndef GRYPHHACKS__UTIL_H_
#define GRYPHHACKS__UTIL_H_

#include <cstdint>
#include <openssl/sha.h>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <pqxx/pqxx>

#include "httplib.h"
#include "json.hpp"

using httplib::Server;
using nlohmann::json;
using namespace std::chrono;

namespace util {
char *get_local_ip();

uint64_t gen_snowflake(uint64_t mid);

uint64_t uid_from_session(const std::string &url, const std::string &uuid);

pqxx::result get_from_sql(const std::string &url, const std::string &s);

void send_to_sql(const std::string &url, const std::string &s);

std::string make_safe(std::string in);

std::string sha256(const std::string &string);

std::string remove_of(std::string in, const std::string &remove = "\t\n\"\\\'");

json read_multipart_form(const httplib::Request &req,
                         httplib::Response &res,
                         const httplib::ContentReader &content_reader);
}

#endif //GRYPHHACKS__UTIL_H_
