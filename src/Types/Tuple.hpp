#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"

class TupleType : public GenericType {
public:
  TupleType(std::vector<GTPtr> sub_types) : sub_types(sub_types) {}
  virtual DataType data_type() const;
  virtual bool operator==(const GenericType &other) const;
  virtual bool operator<(const GenericType &other) const;
  virtual llvm::Type *llvm_type() const;
  virtual GenericValue create(llvm::Value *val) const;

public:
  std::vector<GTPtr> sub_types;
};
