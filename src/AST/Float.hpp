#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "ASTNode.hpp"

class ASTFloat : public ASTNode {
public:
  ASTFloat(float val) : val(val) {}
  virtual GTPtr return_type(Enviroment &env) const;
  virtual DataType type() const;
  virtual std::string as_string() const;
  GenericValue to_value(Enviroment &env, llvm::IRBuilder<> &builder);

private:
  float val;
};