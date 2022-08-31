//
// Created by christian on 5/21/22.
//

#include <uWebSockets/App.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <jwt.h>
#include <picojson/picojson.h>

#include <utility>
#include "util.h"
#include "lib/picosha2.h"

json util::read_multipart_form(Response *res,
                               Request *req) {
//  httplib::MultipartFormDataItems files;
//  content_reader([&](const httplib::MultipartFormData &file) {
//    files.push_back(file);
//    return true;
//  }, [&](const char *data, size_t data_length) {
//    files.back().content.append(data, data_length);
//    return true;
//  });
//  if (files.empty()) {
//    res.set_content("INVALID FORM INPUT", "text/plain");
//    return json::object();
//  }
//  json j = json::object();
//  for (auto &i: files) {
//    j += {i.name, {
//        {"content", remove_of(i.content)},
//        {"content_type", remove_of(i.content_type)},
//        {"file_name", remove_of(i.filename)},
//    }};
//  }
//  return j;
}

char *util::get_local_ip() {
  char host[256];
  int hostname = gethostname(host, sizeof(host));

  if (hostname == -1) std::cout << "Error: Get Host Name" << std::endl;

  struct hostent *host_entry;
  host_entry = gethostbyname(host);

  if (host_entry == nullptr) std::cout << "Error: Get Host Entry" << std::endl;

  char *IP;
  IP = inet_ntoa(*((struct in_addr *) host_entry->h_addr_list[0]));

  return IP;

}

uint64_t seq;
uint64_t util::gen_snowflake(uint64_t mid) {
  uint64_t t = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  uint64_t out = (t << 23) | ((mid << 52) >> 40) | ((seq << 52) >> 52);

  seq++;
  return out;
}

pqxx::result util::get_from_sql(const std::string &s, const std::string &url) {
  std::cout << "sql >> " << s << std::endl;
  pqxx::connection CONN(url);
  pqxx::nontransaction conn(CONN);
  return conn.exec(s);
}

void util::send_to_sql(const std::string &s, const std::string &url) {
  std::cout << "sql << " << s << std::endl;
  pqxx::connection CONN(url);
  pqxx::work c(CONN);
  c.exec(s);
  c.commit();
}

std::string util::make_safe(std::string in) {
  std::string::size_type pos;
  while ((pos = in.find_first_of("\t\n\"\\", pos)) != std::string::npos) {
    switch (in[pos]) {
      case '\t':in.replace(pos, 1, "\\t");
        pos += 2;
        break;
      case '\n':in.replace(pos, 1, "\\n");
        pos += 2;
        break;
      case '\\':in.replace(pos, 1, "\\\\");
        pos += 2;
        break;
      case '\"':in.replace(pos, 1, R"(\\\")");
        pos += 2;
        break;
    }
  }
  return in;
}

std::string util::sha256(const std::string &s) {
  std::string o;
  picosha2::hash256_hex_string(s, o);
  return o;
}

std::string util::remove_of(std::string in, const std::string &remove) {
  for (auto i: remove) {
    auto it = std::remove(in.begin(), in.end(), i);
    in.erase(it, in.end());
  }
  return in;
}

uint64_t util::uid_from_session(const std::string &uuid, const std::string &url) {
  pqxx::result r = get_from_sql("SELECT user_id FROM sessions WHERE uuid='" + uuid + "'", url);
  return r[0][0].as<uint64_t>();
}

auto util::split(const std::string &in, const std::string &delim) -> std::vector<std::string> {
  std::vector<std::string> out;
  int start = 0;
  int end = in.find(delim);
  while (end != std::string::npos) {
    out.emplace_back(in.substr(start, end - start));
    start = end + delim.size();
    end = in.find(delim, start);
  }
  out.push_back(in.substr(start, end - start));
  return out;
}

auto util::get_cookies(const std::string &in) -> json {
  json out;
  for (const auto &x: split(in, "; ")) {
    std::string name = x.substr(0, x.find("="));
    std::string val = x.substr(x.find("=") + 1, x.size() - 1);
    out[name] = val;
  }
  return out;
}

auto util::handle(Response *res, const std::function<std::string(std::string)> &func) -> void {
  res->onAborted([=]() {
    std::cout << "HTTP socket closed before finished!" << std::endl;
  });
  std::string buffer;
  res->onData([res, buffer = std::move(buffer), func](std::string_view data, bool last) mutable {
    buffer += data;

    if (last) {
      res->writeHeader("content-type", "application/json");
      res->end(func(std::string(data)));
    }
  });
}
auto util::gen_token(uint64_t id,
                     const std::string &f_name,
                     const std::string &l_name,
                     bool is_admin,
                     bool is_host,
                     const std::string &email,
                     std::string secret) -> std::string {
  return jwt::create()
      .set_type("JWT")
      .set_algorithm("HS256")
      .set_issued_at(std::chrono::system_clock::now())
      .set_payload_claim("id", picojson::value(std::to_string(id)))
      .set_payload_claim("f_name", picojson::value(f_name))
      .set_payload_claim("l_name", picojson::value(l_name))
      .set_payload_claim("is_admin", picojson::value(is_admin))
      .set_payload_claim("is_host", picojson::value(is_host))
      .set_payload_claim("email", picojson::value(email))
      .set_expires_at(jwt::date(
          std::chrono::system_clock::now()
              + std::chrono::system_clock::duration(2592000s)))
      .sign(jwt::algorithm::hs256{std::move(secret)});
}
