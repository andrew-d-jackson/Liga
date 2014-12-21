#include "Float.hpp"

#include "Enviroment.hpp"
#include "Function.hpp"
#include "AST/Float.hpp"

DataType FloatType::data_type() const { return DataType::Float; }

bool FloatType::operator==(const GenericType &other) const {
  return other.data_type() == data_type();
}

llvm::Type *FloatType::llvm_type() const {
  return llvm::Type::getFloatTy(llvm::getGlobalContext());
}

GenericValue FloatType::create(llvm::Value *val) const {
  return GenericValue(std::make_shared<FloatType>(), val);
}

ASTPtr FloatType::create_ast(llvm::GenericValue gv) const {
	auto val = static_cast<float>(gv.FloatVal);
	return std::make_shared<ASTFloat>(val);
}

GenericValue to_float(llvm::IRBuilder<> &builder, GenericValue val) {
  auto ret_val = val.type->data_type() == DataType::Integer
                     ? builder.CreateSIToFP(val.value, FloatType().llvm_type())
                     : val.value;
  return FloatType().create(ret_val);
}
