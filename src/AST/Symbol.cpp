#include "Symbol.hpp"

#include <iostream>

DataType ASTSymbol::type() const { return DataType::Symbol; }

GTPtr ASTSymbol::return_type(Enviroment &env) const {
  assert_var_exists(env);
  auto found_value = env.value_map[val];
  return found_value.type;
}

GenericValue ASTSymbol::to_value(Enviroment &env, llvm::IRBuilder<> &builder) {
  assert_var_exists(env);
  auto found_value = env.value_map[val];
  return found_value;
}

std::string ASTSymbol::as_string() const { return val; }

void ASTSymbol::assert_var_exists(Enviroment &env) const {
  if (env.value_map.find(val) == env.value_map.end()) {
    std::cerr << "Could not find symbol: " << val << std::endl;
    throw;
    exit(1);
  }
}
