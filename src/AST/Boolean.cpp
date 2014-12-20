#include "Boolean.hpp"

#include "Types/Boolean.hpp"

DataType ASTBoolean::type() const { return DataType::Boolean; }

GTPtr ASTBoolean::return_type(Enviroment &env) const {
  return std::make_shared<BooleanType>();
}

GenericValue ASTBoolean::to_value(Enviroment &env, llvm::IRBuilder<> &builder) {
  auto gen = builder.getInt1(val);
  return BooleanType().create(gen);
}

std::string ASTBoolean::as_string() const {
  return val ? "true" : "false";
};