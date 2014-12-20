#include "Integer.hpp"

#include "Types/Integer.hpp"

GTPtr ASTInteger::return_type(Enviroment &env) const {
  return std::make_shared<IntegerType>();
}

DataType ASTInteger::type() const { return DataType::Integer; }

GenericValue ASTInteger::to_value(Enviroment &env, llvm::IRBuilder<> &builder) {
  auto gen = builder.getInt32(val);
  return IntegerType().create(gen);
}

std::string ASTInteger::as_string() const {
  return std::to_string(val) + "i";
}