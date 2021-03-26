//
// Created by nickmint on 3/19/21.
//

#include "suggester.hpp"

std::shared_mutex suggester::_collection_mutex;
std::unique_ptr<nlohmann::json> suggester::_collection = nullptr;

[[noreturn]] void update_collection (const std::string &filename_json)
{
  const size_t minutes_time = 1;
  std::ifstream file_json;
  while(true){
    suggester::_collection_mutex.lock();
    suggester::_collection = nullptr;
    suggester::_collection = std::make_unique<nlohmann::json>(nlohmann::json());
    file_json.open(filename_json);
    file_json >> *(suggester::_collection);
    file_json.close();
    suggester::_collection_mutex.unlock();
    std::cout << suggester::_collection->dump(4);
    std::this_thread::sleep_for(std::chrono::minutes(minutes_time));
  }
}