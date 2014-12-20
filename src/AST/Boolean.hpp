#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "ASTNode.hpp"

class ASTBoolean : public ASTNode {
public:
  ASTBoolean(bool val) : val(val) {}
  virtual DataType type() const;
  virtual GTPtr return_type(Enviroment &env) const;
  virtual std::string as_string() const;
  GenericValue to_value(Enviroment &env, llvm::IRBuilder<> &builder);

private:
	bool val;
};