#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"

#include <string>
#include <memory>

class Enviroment;

class ASTNode {
public:
  virtual GenericValue to_value(Enviroment &, llvm::IRBuilder<> &) = 0;
  virtual DataType type() const = 0;
  virtual std::string as_string() const = 0;
  virtual GTPtr return_type(Enviroment &) const = 0;
  virtual bool is_pure(Enviroment &) const {return true;}
};

using ASTPtr = std::shared_ptr<ASTNode>;
using ASTList = std::vector<ASTPtr>;
