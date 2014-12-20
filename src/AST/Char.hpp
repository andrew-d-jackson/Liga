#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "ASTNode.hpp"

class ASTChar : public ASTNode {
public:
  ASTChar(char val) : val(val) {}
  virtual GTPtr return_type(Enviroment &env) const;
  virtual DataType type() const;
  virtual std::string as_string() const;
  GenericValue to_value(Enviroment &env, llvm::IRBuilder<> &builder);

private:
	char val;
};