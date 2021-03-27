//
// Created by nickmint on 3/19/21.
//

#include "suggester.hpp"

std::shared_mutex suggester::_collection_mutex;
std::unique_ptr<nlohmann::json> suggester::_collection = nullptr;

std::string suggester::parse_request(const std::string &request) {
  nlohmann::json req;
  try {
    req = nlohmann::json::parse(request);
  } catch (const nlohmann::detail::parse_error& e) {
    throw std::runtime_error("Not json input");
  }
  if (req.contains("input")){
    return req["input"];
  } else {
    throw std::runtime_error("Invalid fields in json input");
  }
}

std::string suggester::suggest(const std::string &input) {
  try {
    nlohmann::json suggestion;
    suggestion["suggestions"] = nlohmann::json::array();
    _collection_mutex.lock_shared();
    for (const auto& elem : *_collection){
      if (elem["id"] == input){
        suggestion["suggestions"].push_back(nlohmann::json{
            {"text", elem["name"]},
            {"cost", elem["cost"]}});
      }
    }
    _collection_mutex.unlock_shared();
    std::sort(suggestion["suggestions"].begin(),
              suggestion["suggestions"].end(),
              [](const nlohmann::json& a, const nlohmann::json& b) -> bool
              {
                return a["cost"] < b["cost"];
              }
    );
    size_t position = 0;
    for (auto& elem: suggestion["suggestions"]){
      elem["position"] = position;
      elem.erase("cost");
      ++position;
    }
    return suggestion.dump(4);
  } catch (const nlohmann::detail::parse_error &e) {
    throw std::runtime_error("Internal json error");
  }
}

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