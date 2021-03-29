//Copyright [2021] <Copyright nickgeo.winner@gmail.com>

#ifndef INCLUDE_SUGGESTER_HPP_
#define INCLUDE_SUGGESTER_HPP_
#include "nlohmann/json.hpp"
#include <iostream>
#include <thread>
#include <fstream>
#include <shared_mutex>
#include <sstream>
#include <ostream>
#include <iomanip>
#include <memory>
#include <string>

class suggester {
 public:
  static std::string parse_request(const std::string &request);
  static std::string request();
  static void parse_suggest(const std::string& response_json,
                                   std::ostream& out);
  static std::string suggest(const std::string &input);

 public:
  static std::unique_ptr<nlohmann::json>  _collection;
  static std::shared_mutex _collection_mutex;
};

[[noreturn]] void update_collection (const std::string &filename_json);

#endif  // INCLUDE_SUGGESTER_HPP_
