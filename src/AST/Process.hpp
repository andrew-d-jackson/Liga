#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "ASTNode.hpp"

class ASTProcess : public ASTNode {
public:
  ASTProcess() : val() {}
  ASTProcess(const ASTProcess &other) : val(other.val) {}
  ASTProcess(std::vector<std::shared_ptr<ASTNode>> val) : val(val) {}
  virtual DataType type() const;
  bool is_prefix_call(Enviroment &env, DataType ty) const;
  bool is_infix_func_call(Enviroment &env) const;
  std::vector<GTPtr> get_value_types(Enviroment &env) const;
  std::vector<GenericValue> generate_values(Enviroment &env,
	  llvm::IRBuilder<> &builder) const;
  GenericValue to_value(Enviroment &env, llvm::IRBuilder<> &builder);
  virtual GTPtr return_type(Enviroment &env) const;
  virtual std::string as_string() const;

public:
	std::vector<std::shared_ptr<ASTNode>> val;
};
