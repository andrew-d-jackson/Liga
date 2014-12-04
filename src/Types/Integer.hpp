#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "Function.hpp"
#include "RefrenceCounter.hpp"

class IntegerType : public GenericType {
public:
  virtual DataType data_type() const { return DataType::Integer; }
  virtual bool operator==(const GenericType &other) const {
    return other.data_type() == data_type();
  }
  virtual llvm::Type *llvm_type() const {
    return llvm::IntegerType::getInt32Ty(llvm::getGlobalContext());
  }
  virtual GenericValue create(llvm::Value *val) const {
    return GenericValue(std::make_shared<IntegerType>(), val);
  }
};