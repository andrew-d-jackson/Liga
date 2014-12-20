#include "Float.hpp"

#include "Types/Float.hpp"

DataType ASTFloat::type() const { return DataType::Float; }

GTPtr ASTFloat::return_type(Enviroment &env) const {
  return std::make_shared<FloatType>();
}

GenericValue ASTFloat::to_value(Enviroment &env, llvm::IRBuilder<> &builder) {
  auto gen = llvm::ConstantFP::get(return_type(env)->llvm_type(), val);
  return FloatType().create(gen);
}

std::string ASTFloat::as_string() const {
  return std::to_string(val) + "f";
}