//
// Created by nickmint on 3/19/21.
//

#include "suggester.hpp"

std::shared_mutex suggester::_collection_mutex;
std::unique_ptr<nlohmann::json> suggester::_collection = nullptr;

const std::string suggestions_str = "suggestions";
const std::string input_str = "input";
const std::string position_str = "position";
const std::string name_str = "name";
const std::string id_str = "id";
const std::string cost_str = "cost";
const std::string text_str = "text";

std::string suggester::parse_request(const std::string &request) {
  nlohmann::json req;
  try {
    req = nlohmann::json::parse(request);
  } catch (const nlohmann::detail::parse_error& e) {
    throw std::runtime_error("Not json input");
  }
  if (req.contains(input_str)){
    return req[input_str];
  } else {
    throw std::runtime_error("Invalid fields in json input");
  }
}

std::string suggester::suggest(const std::string &input) {
  try {
    nlohmann::json suggestion;
    suggestion[suggestions_str] = nlohmann::json::array();
    _collection_mutex.lock_shared();
    for (const auto& elem : *_collection){
      if (elem[id_str] == input){
        suggestion[suggestions_str].push_back(nlohmann::json{
            {text_str, elem[name_str]},
            {cost_str, elem[cost_str]}});
      }
    }
    _collection_mutex.unlock_shared();
    std::sort(suggestion[suggestions_str].begin(),
              suggestion[suggestions_str].end(),
              [](const nlohmann::json& a, const nlohmann::json& b) -> bool
              {
                return a[cost_str] < b[cost_str];
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

std::string suggester::request() {
  nlohmann::json req;
  std::cout << "Enter a message: ";
  std::string input;
  std::cin >> input;
  req[input_str] = input;
  return req.dump();
}

void suggester::parse_suggest(const std::string& response_json,
                                     std::ostream& out) {
  nlohmann::json res;
  try {
    res = nlohmann::json::parse(response_json);
  } catch (const nlohmann::detail::parse_error& e) {
    throw std::runtime_error("Not json response");
  }
  if (!res[suggestions_str].empty())
    out << "Maybe you wanted to type: " << std::endl;
  else
    out << "No suggestions for this input" << std::endl;
  size_t count = 1;
  for (const auto& elem : res[suggestions_str]){
    out << count << ")" << std::setw(4)  << elem[text_str] << std::endl;
    ++count;
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