#include "Integer.hpp"

#include "AST/Integer.hpp"

DataType IntegerType::data_type() const { return DataType::Integer; }

bool IntegerType::operator==(const GenericType &other) const {
  return other.data_type() == data_type();
}

llvm::Type *IntegerType::llvm_type() const {
  return llvm::IntegerType::getInt32Ty(llvm::getGlobalContext());
}

GenericValue IntegerType::create(llvm::Value *val) const {
  return GenericValue(std::make_shared<IntegerType>(), val);
}

GenericValue IntegerType::create(int val) const {
  auto v = llvm::ConstantInt::get(
      llvm::IntegerType::getInt32Ty(llvm::getGlobalContext()), val);
  return GenericValue(std::make_shared<IntegerType>(), v);
}

ASTPtr IntegerType::create_ast(llvm::GenericValue gv) const {
	auto val = gv.IntVal.getSExtValue();
	return std::make_shared<ASTInteger>(val);
}