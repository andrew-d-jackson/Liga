#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "Function.hpp"

class VoidType : public GenericType {
public:
  virtual DataType data_type() const { return DataType::Void; }
  virtual bool operator==(const GenericType &other) const {
    return other.data_type() == data_type();
  }
  virtual llvm::Type *llvm_type() const {
    return llvm::Type::getVoidTy(llvm::getGlobalContext());
  }
  virtual GenericValue create() const {
    return GenericValue(std::make_shared<VoidType>(), nullptr);
  }
  virtual GenericValue create(llvm::Value *val) const {
    return GenericValue(std::make_shared<VoidType>(), nullptr);
  }
};