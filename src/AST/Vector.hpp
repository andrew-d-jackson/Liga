#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "ASTNode.hpp"

class ASTVector : public ASTNode {
public:
  ASTVector(std::vector<std::shared_ptr<ASTNode>> val) : val(val) {}
  virtual DataType type() const;
  virtual GTPtr return_type(Enviroment &env) const;
  GenericValue to_value(Enviroment &env, llvm::IRBuilder<> &builder);
  virtual std::string as_string() const;
  virtual bool is_pure(Enviroment &env) const;

public:
	std::vector<std::shared_ptr<ASTNode>> val;
};
