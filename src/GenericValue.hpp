#pragma once
#include "LLVM.hpp"

class Function;
class Macro;
class GenericType;

class GenericValue {
public:
  GenericValue()
      : type(nullptr), value(nullptr), func(nullptr), macro(nullptr) {}

  GenericValue(std::shared_ptr<GenericType> type, llvm::Value *value)
      : type(type), value(value) {}

  GenericValue(std::shared_ptr<GenericType> type, std::shared_ptr<Function> f)
      : type(type), func(f) {}

  GenericValue(std::shared_ptr<GenericType> type, std::shared_ptr<Macro> m)
      : type(type), macro(m) {}

  GenericValue(const GenericValue &other)
      : type(other.type), func(other.func), value(other.value),
        macro(other.macro) {}

  void operator==(const GenericValue &other) {
    type = other.type;
    func = other.func;
    value = other.value;
    macro = other.macro;
  }

  std::shared_ptr<GenericType> type;
  std::shared_ptr<Function> func;
  std::shared_ptr<Macro> macro;
  llvm::Value *value;
};
