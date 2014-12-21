#include "Boolean.hpp"

#include "AST/Boolean.hpp"


DataType BooleanType::data_type() const { return DataType::Boolean; }

bool BooleanType::operator==(const GenericType &other) const {
  return other.data_type() == data_type();
}

llvm::Type *BooleanType::llvm_type() const {
  return llvm::IntegerType::getInt1Ty(llvm::getGlobalContext());
}

GenericValue BooleanType::create(llvm::Value *val) const {
  return GenericValue(std::make_shared<BooleanType>(), val);
}

ASTPtr BooleanType::create_ast(llvm::GenericValue gv) const {
	bool val = static_cast<bool>(gv.IntVal.getZExtValue());
	return std::make_shared<ASTBoolean>(val);
}
