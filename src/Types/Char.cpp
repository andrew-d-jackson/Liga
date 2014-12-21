#include "Char.hpp"

#include "AST/Char.hpp"

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


ASTPtr CharType::create_ast(llvm::GenericValue gv) const {
	auto val = static_cast<char>(gv.IntVal.getZExtValue());
	return std::make_shared<ASTChar>(val);
}
