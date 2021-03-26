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

class suggester {
 public:

  static std::unique_ptr<nlohmann::json>  _collection;
  [[maybe_unused]] static std::shared_mutex _collection_mutex;
};

[[noreturn]] void update_collection (const std::string &filename_json);

#endif  // SUGGESTION_SUGGESTER_HPP