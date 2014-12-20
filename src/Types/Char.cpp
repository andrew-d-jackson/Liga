#include "Char.hpp"

DataType CharType::data_type() const { return DataType::Char; }

bool CharType::operator==(const GenericType &other) const {
  return other.data_type() == data_type();
}

llvm::Type *CharType::llvm_type() const {
  return llvm::IntegerType::getInt8Ty(llvm::getGlobalContext());
}

GenericValue CharType::create(llvm::Value *val) const {
  return GenericValue(std::make_shared<CharType>(), val);
}
