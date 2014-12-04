#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "ASTNode.hpp"
#include "Types/Float.hpp"

class ASTFloat : public ASTNode {
private:
  float val;

public:
  ASTFloat(float val) : val(val) {}

  virtual GTPtr return_type(Enviroment &env) const {
    return std::make_shared<FloatType>();
  }

  virtual DataType type() const { return DataType::Float; }

  GenericValue to_value(Enviroment &env, llvm::IRBuilder<> &builder) {
    auto gen = llvm::ConstantFP::get(return_type(env)->llvm_type(), val);
    return FloatType().create(gen);
  };

  virtual std::string as_string() const { return std::to_string(val) + "f"; }
};