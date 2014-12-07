#pragma once
#include "LLVM.hpp"

#include <vector>
#include <memory>

class Enviroment;

enum class DataType {
  Integer,
  Void,
  Float,
  Boolean,
  Vector,
  Char,
  Symbol,
  Process,
  Tuple,
  Function,
  Macro
};

class GenericType {
public:
  virtual bool operator==(const GenericType &other) const = 0;
  virtual bool operator<(const GenericType &other) const {
    return data_type() < other.data_type();
  };
  virtual bool is_type_of(const GenericValue &other) const {
    return *this == *other.type;
  }
  virtual DataType data_type() const = 0;
  virtual llvm::Type *llvm_type() const = 0;
  virtual GenericValue copy(Enviroment &env, llvm::IRBuilder<> &builder,
                            GenericValue val) const {
    return val;
  };
  virtual void destroy(Enviroment &env, llvm::IRBuilder<> &builder,
                       GenericValue val) const {};
  virtual GenericValue create(llvm::Value *val) const = 0;
};

using GTPtr = std::shared_ptr<GenericType>;
using GTList = std::vector<GTPtr>;

class GTPtrComparison {
public:
  bool operator()(const GTPtr &a, const GTPtr &b) const { return (*a < *b); }
};

class GTListComparison {
public:
  bool operator()(const GTList &a, const GTList &b) const {
    if (a.size() != b.size())
      return a.size() < b.size();
    for (int i = 0; i < a.size(); i++) {
      if (*a.at(0) < *b.at(0))
        return true;
    }
    return false;
  }
};

class GTListEquality {
public:
  bool operator()(const GTList &a, const GTList &b) const {
    if (a.size() != b.size())
      return false;
    for (int i = 0; i < a.size(); i++) {
      if (!(*a.at(0) == *b.at(0)))
        return false;
    }
    return true;
  }
};