//
// Created by christian on 8/25/22.
//

#ifndef GRYPHHACKS_ENDPOINTS_EVENT_H_
#define GRYPHHACKS_ENDPOINTS_EVENT_H_

#include <pqxx/result>

#include "../lib/json.hpp"
#include "../util.h"

#define STR(in) std::string(in)

namespace event {

typedef uWS::HttpResponse<false> Response;
typedef uWS::HttpRequest Request;
using json = nlohmann::json;

auto event_create(Response *res, Request *req) {
  util::handle(res, [req](auto body) -> std::string {
    json j = json::parse(body);
    std::cout << j << std::endl;
    auto cookies = util::get_cookies(std::string(req->getHeader("cookie")));
    try {
      auto event_id = util::gen_snowflake(10) >> 1;
      std::stringstream ss;
      ss << "INSERT INTO events (id, user_id, min_age, name, description, lat, long, time_start, time_end, "
            "location, email, is_recurring) VALUES ("
         << event_id << ","
         << util::uid_from_session(cookies["session"]) << ","
         << j["min_age"].get<int>() << ",'"
         << j["name"].get<std::string>() << "','"
         << j["description"].get<std::string>() << "',"
         << j["lat"].get<float>() << ","
         << j["long"].get<float>() << ", TIMESTAMP '"
         << j["time_start"].get<std::string>() << "', TIMESTAMP '"
         << j["time_end"].get<std::string>() << "','"
         << j["location"].get<std::string>() << "', '"
         << j["email"].get<std::string>() << "', "
         << (j["is_recurring"].get<bool>() ? "true" : "false")
         << ")";
      util::send_to_sql(ss.str());
      return R"({"id":")" + std::to_string(event_id) + "\"}";
    } catch (const std::exception &e) {
      std::cout << e.what() << std::endl;
      return R"({"value": "invalid POST body"})";
    }
  });

}

auto event_get(Response *res, Request *req) {
  util::handle(res, [req](auto body) -> std::string {

    std::string id{req->getParameter(0)};
    auto cookies = util::get_cookies(STR(req->getHeader("cookie")));
    auto r = util::get_from_sql("SELECT id, user_id, min_age, is_recurring, name, description, long, lat, "
                                "time_start, time_end, location, email "
                                "FROM events WHERE id='" + id + "'");
    if (r.empty()) return R"({"value": "Invalid event"})";

    json j = {
        {"id", r[0][0].as<std::string>()},
        {"user_id", r[0][1].as<std::string>()},
        {"min_age", r[0][2].as<std::string>()},
        {"is_recurring", r[0][3].as<std::string>()},
        {"name", r[0][4].as<std::string>()},
        {"description", r[0][5].as<std::string>()},
        {"long", r[0][6].as<std::string>()},
        {"lat", r[0][7].as<std::string>()},
        {"time_start", r[0][8].as<std::string>()},
        {"time_end", r[0][9].as<std::string>()},
        {"location", r[0][10].as<std::string>()},
        {"email", r[0][11].as<std::string>()},
    };
    return j.dump();
  });
}

auto event_get_all(Response *res, Request *) {
  util::handle(res, [](auto body) -> std::string {
    pqxx::result r = util::get_from_sql(
        "SELECT id, user_id, min_age, is_recurring, name, description, long, lat, "
        "time_start, time_end, location, email "
        "FROM events");

    json j = json::array();
    for (auto row: r) {
      j += {
          {"id", row[0].as<std::string>()},
          {"user_id", row[1].as<std::string>()},
          {"min_age", row[2].as<std::string>()},
          {"is_recurring", row[3].as<std::string>()},
          {"name", row[4].as<std::string>()},
          {"description", row[5].as<std::string>()},
          {"long", row[6].as<std::string>()},
          {"lat", row[7].as<std::string>()},
          {"time_start", row[8].as<std::string>()},
          {"time_end", row[9].as<std::string>()},
          {"location", row[10].as<std::string>()},
          {"email", row[11].as<std::string>()},
      };
    }
    return j.dump();
  });
}

auto event_get_loc(Response *res, Request *) {
  util::handle(res, [](auto body) -> std::string {
    pqxx::result r = util::get_from_sql("SELECT id, name, location, lat, long FROM events");

    json j = json::array();
    for (auto row: r) {
      j += {
          {"id", row[0].as<std::string>()},
          {"name", row[1].as<std::string>()},
          {"location", row[2].as<std::string>()},
          {"lat", row[3].as<std::string>()},
          {"long", row[4].as<std::string>()},
      };
    }
    return j.dump();
  });
}

auto event_get_rsvp(Response *res, Request *req) {
  util::handle(res, [req](auto body) -> std::string {
    std::string id{req->getParameter(0)};
    auto cookies = util::get_cookies(std::string(req->getHeader("cookie")));
    pqxx::result r = util::get_from_sql("SELECT user_id FROM event_rsvp WHERE event_id='"
                                            + id
                                            + "'");
    json j = std::vector<std::string>();
    for (auto i: r) {
      j += i[0].as<uint64_t>();
    }
    return j.dump();
  });
}

auto event_set_rsvp(Response *res, Request *req) {
  util::handle(res, [req](auto b) {
    try {
      auto cookies = util::get_cookies(std::string(req->getHeader("cookie")));
      json body = json::parse(b);
      std::string event_id{req->getParameter(0)};
      uint64_t user_id = util::uid_from_session(cookies["session"]);
      if (body["rsvp"]) {
        pqxx::result r = util::get_from_sql("SELECT COUNT(1) FROM event_rsvp WHERE user_id='"
                                                + std::to_string(user_id)
                                                + "' AND event_id='"
                                                + event_id
                                                + "'");
        if (!r[0][0].as<bool>()) {
          util::send_to_sql("INSERT INTO event_rsvp (event_id, user_id) VALUES ("
                                + event_id + ","
                                + std::to_string(user_id) + ")");
        }
      } else {
        util::send_to_sql("DELETE FROM event_rsvp WHERE user_id='"
                              + std::to_string(user_id)
                              + "'");
      }
      return "{}";
    } catch (std::exception &e) {
      return R"({"value": "Invalid POST body or invalid session"})";
    }
  });
}

}

#endif //GRYPHHACKS_ENDPOINTS_EVENT_H_
