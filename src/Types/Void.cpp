#include "Void.hpp"

DataType VoidType::data_type() const { return DataType::Void; }

bool VoidType::operator==(const GenericType &other) const {
  return other.data_type() == data_type();
}

llvm::Type *VoidType::llvm_type() const {
  return llvm::Type::getVoidTy(llvm::getGlobalContext());
}

GenericValue VoidType::create() const {
  return GenericValue(std::make_shared<VoidType>(), nullptr);
}

GenericValue VoidType::create(llvm::Value *val) const {
  return GenericValue(std::make_shared<VoidType>(), nullptr);
}
