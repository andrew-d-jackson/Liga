#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "ASTNode.hpp"

class ASTSymbol : public ASTNode {
public:
  ASTSymbol(std::string val) : val(val) {}
  virtual DataType type() const;
  virtual GTPtr return_type(Enviroment &env) const;
  GenericValue to_value(Enviroment &env, llvm::IRBuilder<> &builder);
  virtual std::string as_string() const;
  void assert_var_exists(Enviroment &env) const;

public:
	std::string val;
};
