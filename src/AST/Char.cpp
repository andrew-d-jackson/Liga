#include "Char.hpp"

#include "Types/Char.hpp"

DataType ASTChar::type() const { return DataType::Char; }

GTPtr ASTChar::return_type(Enviroment &env) const {
  return std::make_shared<CharType>();
}

GenericValue ASTChar::to_value(Enviroment &env, llvm::IRBuilder<> &builder) {
  auto gen = builder.getInt8(val);
  return CharType().create(gen);
}

std::string ASTChar::as_string() const {
  auto x = std::string("");
  x.push_back(val);
  return x;
}
