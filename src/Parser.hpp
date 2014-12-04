#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "ASTNode.hpp"
#include "Enviroment.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>

bool is_integer(std::string string) {
    if (string.size() < 1)
        return false;

    if (string.find_first_of("0123456789") == std::string::npos)
        return false;

    for (auto it = string.begin(); it < string.end(); it++) {
        if (!(isdigit(*it) || (*it == '-' && it == string.begin())))
            return false;
    }
    return true;
}

bool is_float(std::string string) {
    if (string.size() < 1)
        return false;

    if (string.find_first_of("0123456789") == std::string::npos)
        return false;

    if (string.find_first_of(".") == std::string::npos)
        return false;

    for (auto it = string.begin(); it < string.end(); it++) {
        if (!isdigit(*it)) {
            if (*it == '-') {
                if (it == string.begin()) return false;
            } else if (*it != '.') {
                return false;
            }
        }
    }

    return true;
}

ASTPtr parse_symbol(std::string str) {
  if (str == "true") {
    return std::make_shared<ASTBoolean>(true);
  }
  if (str == "false") {
    return std::make_shared<ASTBoolean>(false);
  }
  if (is_float(str)) {
      return std::make_shared<ASTFloat>(atof(str.c_str()));
  }
  if (is_integer(str)) {
    return std::make_shared<ASTInteger>(atoi(str.c_str()));
  }
  return std::make_shared<ASTSymbol>(str);
}

static const std::vector<char> seperators = {'[', ']', '(', ')', '{', '}', ' ', '\n', '"'};
static const std::vector<char> whitespace = {' ', '"'};


bool is_seperator(char c) {
    auto find = std::find(seperators.begin(), seperators.end(), c);
    return (find != seperators.end());
}

bool is_whitespace(char c) {
    auto find = std::find(whitespace.begin(), whitespace.end(), c);
    return (find != whitespace.end());
}


ASTList parse(std::string str);
template <class T>
void parse_list(ASTList &lst, std::string &str, char opening, char closing){
    std::size_t pos = 1;
    std::size_t sub_lists = 0;
    char to_check = str.at(1);
    while (!(to_check == closing && sub_lists == 0)) {
        if (to_check == opening)
            ++sub_lists;
        if (to_check == closing)
            --sub_lists;
        to_check = str.at(++pos);
    }
    auto sub_str = std::string(str.begin() + 1, str.begin() + pos);
    auto sub_result = parse(sub_str);
    lst.push_back(std::make_shared<T>(sub_result));
    str = std::string(str.begin() + pos + 1, str.end());
}

ASTList parse(std::string str) {
  ASTList lst;
  while (str.size() > 0) {
    if (str.at(0) == '[') {
        parse_list<ASTProcess>(lst, str, '[', ']');
    } else if (str.at(0) == '(') {
        parse_list<ASTVector>(lst, str, '(', ')');
    } else if (str.at(0) == '{') {
        parse_list<ASTTuple>(lst, str, '{', '}');
    } else if (is_whitespace(str.at(0))) {
        str = std::string(str.begin() + 1, str.end());
    } else if (str.at(0) == '"') {
        auto str_end_pos = std::find(str.begin() + 1, str.end(), '"');
        auto sub_str = std::string(str.begin() + 1, str_end_pos);
        if (sub_str.size() == 1) {
            lst.push_back(std::make_shared<ASTChar>(sub_str.at(0)));
        } else if (sub_str.size() > 1) {
            std::vector<ASTPtr> chars;
            for (auto &c : sub_str) chars.push_back(std::make_shared<ASTChar>(c));
            lst.push_back(std::make_shared<ASTVector>(chars));
        }
        str = std::string(str_end_pos+1, str.end());
    } else {
      std::size_t pos = 0;
      for (; pos < str.size(); ++pos) {
        char to_check = str.at(pos);
        if (is_seperator(to_check))
          break;
      }
      auto parsed = parse_symbol(std::string(str.begin(), str.begin() + pos));
      lst.push_back(parsed);
      str = std::string(str.begin() + pos, str.end());
    }
  }
  return lst;
}

ASTList parse_file(const std::string file_location) {
    std::ifstream file_stream(file_location.c_str());
    if (!((bool)file_stream)) {
        throw "File Does Not Exist: " + file_location;
    }
    std::stringstream string_buffer;
    string_buffer << file_stream.rdbuf();
    return parse(string_buffer.str());
}
