#include "Float.hpp"

#include "Enviroment.hpp"
#include "Function.hpp"

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

GenericValue to_float(llvm::IRBuilder<> &builder, GenericValue val) {
  auto ret_val = val.type->data_type() == DataType::Integer
                     ? builder.CreateSIToFP(val.value, FloatType().llvm_type())
                     : val.value;
  return FloatType().create(ret_val);
}