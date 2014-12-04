#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "ASTNode.hpp"

class ASTInteger : public ASTNode {
public:
  unsigned val;

public:
  ASTInteger(unsigned val) : val(val) {}

  virtual GTPtr return_type(Enviroment &env) const {
    return std::make_shared<IntegerType>();
  }

  virtual DataType type() const { return DataType::Integer; }

  GenericValue to_value(Enviroment &env, llvm::IRBuilder<> &builder) {
    auto gen = builder.getInt32(val);
    return IntegerType().create(gen);
  };

  virtual std::string as_string() const { return std::to_string(val) + "i"; }
};