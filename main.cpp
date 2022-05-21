#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <iostream>
#include <string>
#include <chrono>
#include "httplib.h"
#include "json.hpp"
#include <pqxx/pqxx>
#include <chrono>

#define RES [](const httplib::Request &req, httplib::Response &res)
#define REQ [](const httplib::Request &req, httplib::Response &)
#define SQL_URL "postgresql://hecker:HIh05t8y3KT7LJC7EtDmag@free-tier11.gcp-us-east1.cockroachlabs" \
".cloud:26257/test?sslmode=verify-full&options=--cluster%3Dchill-whale-766"
#define SQL_CONN pqxx::connection CONN(SQL_URL); pqxx::nontransaction conn(CONN)

/*
 * TODO:
 * POST REQUESTS:
 * login
 * add_event
 * update_profile
 * update_event
 * user_create
 * org_create
 * search
 */

using httplib::Server;
using nlohmann::json;
using namespace std::chrono;

uint64_t seq;

int64_t snowflake_gen(uint64_t mid) {
  uint64_t t = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  uint64_t out = (t << 23) | ((mid << 52) >> 40) | ((seq << 52) >> 52);

  seq++;
  return out;
}

pqxx::result get_from_sql(const std::string &s) noexcept {
  // unsafe lol
//  try {
  SQL_CONN;
  return conn.exec(s);
//  } catch (std::exception &e) {
//    std::cerr << e.what() << std::endl;
//  }
}

std::string make_safe(std::string in) {
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

char *sha256(const char *str, size_t length) {
  int n;
  SHA256_CTX c;
  unsigned char digest[SHA256_DIGEST_LENGTH];
  char *out = (char *) malloc(33);
  SHA256_Init(&c);

  while (length > 0) {
    if (length > 512) SHA256_Update(&c, str, 512);
    else SHA256_Update(&c, str, length);

    length -= 512;
    str += 512;
  }
  SHA256_Final(digest, &c);

  for (n = 0; n < SHA256_DIGEST_LENGTH; ++n) snprintf(&(out[n * 2]), 16 * 2, "%02x", (unsigned int) digest[n]);
  return out;
}

std::string remove_of(std::string in, std::string remove = "\t\n\"\\") {
  for (auto i: remove) {
    auto it = std::remove(in.begin(), in.end(), i);
    auto r = std::distance(it, in.end());
    in.erase(it, in.end());
  }
  return in;
}

int main() {
  Server svr;  //("./ignore/cert.pem", "./ignore/key.pem");
  snowflake_gen(22);

  // region api requests

  svr.Get(R"(/api/usr/(\d+))", RES {
    auto id = req.matches[1];
    pqxx::result r = get_from_sql("SELECT id, f_name, l_name, is_host from users where id=" + std::string(id));

    json j = json::object();
    j += {
        {"id", r[0][0].as<int>()},
        {"f_name", r[0][1].as<std::string>()},
        {"l_name", r[0][2].as<std::string>()},
        {"is_host", r[0][3].as<bool>()},
    };
    res.set_content(j.dump(), "application/json");
  });

  svr.Get("/api/events/loc", RES {
    pqxx::result r = get_from_sql("SELECT id, name, lat, long FROM events");

    json j = json::array();
    for (auto row: r) {
      j += {
          {"id", row[0].as<int>()},
          {"name", row[1].as<std::string>()},
          {"location", row[2].as<std::string>()},
          {"lat", row[3].as<float>()},
          {"long", row[4].as<float>()},
      };
    }
    res.set_content(j.dump(), "application/json");
//    res.set_content("yeet", "text/plain");
  });

  svr.Get("/api/events/all", RES {
    pqxx::result r = get_from_sql("SELECT id, user_id, min_age, is_recurring, name, description, long, lat, "
                                  "time_start, time_end FROM events");

    json j = json::array();
    for (auto row: r) {
      j += {
          {"id", row[0].as<int>()},
          {"user_id", row[1].as<int>()},
          {"min_age", row[2].as<int>()},
          {"is_recurring", row[3].as<bool>()},
          {"name", row[4].as<std::string>()},
          {"description", row[5].as<std::string>()},
          {"long", row[6].as<float>()},
          {"lat", row[7].as<float>()},
          {"time_start", row[8].as<std::string>()},
          {"time_end", row[9].as<std::string>()},
      };
    }
    res.set_content(j.dump(), "application/json");
  });

  svr.Post("/api/login", [](
      const httplib::Request &req,
      httplib::Response &res,
      const httplib::ContentReader &content_reader
  ) {
    if (req.is_multipart_form_data()) {
      httplib::MultipartFormDataItems files;
      content_reader([&](const httplib::MultipartFormData &file) {
        files.push_back(file);
        return true;
      }, [&](const char *data, size_t data_length) {
        files.back().content.append(data, data_length);
        return true;
      });
      if (files.empty()) {
        res.set_content("INVALID FORM INPUT", "text/plain");
        return;
      }
      json j = json::object();
      for (auto &i: files) {
        j += {i.name, {
            {"content", remove_of(i.content)},
            {"content_type", remove_of(i.content_type)},
            {"file_name", remove_of(i.filename)},
        }};
      }
      std::stringstream ss;
      std::cout << j["email"]["content"].get<std::string>() << std::endl;
      try {
        pqxx::result r = get_from_sql(
            "SELECT passwd, salt FROM users WHERE email=\'"
                + j["email"]["content"].get<std::string>()
                + "\'");

        std::string to_hash = j["password"]["content"].get<std::string>() + std::to_string(r[0][1].as<int>());
        std::string hashed = std::string(sha256(to_hash.c_str(), to_hash.length()));
        std::cout << hashed << std::endl;
        if (r[0][0].as<std::string>() != hashed) throw std::exception();
      } catch (const std::exception &e) {
        res.set_content("INVALID EMAIL OR PASSWORD", "text/plain");
        return;
      }
    } else {
      res.set_content("INVALID FORM INPUT", "text/plain");
    }
  });

  //endregion

  svr.set_logger([](const auto &req, const auto &res) {
    std::cout << "req: " << req.body << "\t-\tres: " << res.body << std::endl;
  });
  svr.set_read_timeout(5, 0); // 5 seconds
  svr.set_write_timeout(5, 0); // 5 seconds
  svr.listen("127.0.0.1", 8080);
  return 0;
}
