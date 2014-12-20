#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "ASTNode.hpp"

class ASTInteger : public ASTNode {
public:
  ASTInteger(unsigned val) : val(val) {}
  virtual GTPtr return_type(Enviroment &env) const;
  virtual DataType type() const;
  GenericValue to_value(Enviroment &env, llvm::IRBuilder<> &builder);
  virtual std::string as_string() const;

public:
	unsigned val;
};