//
// Created by nickmint on 3/19/21.
//

#ifndef SUGGESTION_SUGGESTER_HPP
#define SUGGESTION_SUGGESTER_HPP
#include "nlohmann/json.hpp"
#include <iostream>
#include <thread>
#include <fstream>
#include <shared_mutex>
#include <sstream>
#include <ostream>
#include <iomanip>

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

#endif  // SUGGESTION_SUGGESTER_HPP