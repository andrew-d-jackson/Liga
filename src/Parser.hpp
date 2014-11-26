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

ASTList parse(std::string str) {
  ASTList lst;
  while (str.size() > 0) {
    if (str.at(0) == '[') {
      std::size_t pos = 1;
      std::size_t sub_lists = 0;
      char to_check = str.at(1);
      while (!(to_check == ']' && sub_lists == 0)) {
        if (to_check == '[')
          ++sub_lists;
        if (to_check == ']')
          --sub_lists;
        to_check = str.at(++pos);
      }
      auto sub_str = std::string(str.begin() + 1, str.begin() + pos);
      auto sub_result = parse(sub_str);
      lst.push_back(std::make_shared<ASTProcess>(sub_result));
      str = std::string(str.begin() + pos + 1, str.end());
    } else if (str.at(0) == '(') {
      std::size_t pos = 1;
      std::size_t sub_lists = 0;
      char to_check = str.at(1);
      while (!(to_check == ')' && sub_lists == 0)) {
        if (to_check == '(')
          ++sub_lists;
        if (to_check == ')')
          --sub_lists;
        to_check = str.at(++pos);
      }
      auto sub_str = std::string(str.begin() + 1, str.begin() + pos);
      auto sub_result = parse(sub_str);
      lst.push_back(std::make_shared<ASTVector>(sub_result));
      str = std::string(str.begin() + pos + 1, str.end());
    } else if (str.at(0) == ' ' || str.at(0) == '\n') {
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
        if (to_check == '[' || to_check == ']' || to_check == '(' ||
            to_check == ')' || to_check == ' ' || to_check == '\n')
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
