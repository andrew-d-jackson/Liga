#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "Function.hpp"
#include "RefrenceCounter.hpp"

class TupleType : public GenericType {
public:
  std::vector<GTPtr> sub_types;

  TupleType(std::vector<GTPtr> sub_types) : sub_types(sub_types) {}

  virtual DataType data_type() const { return DataType::Tuple; }

  virtual bool operator==(const GenericType &other) const {
    if (other.data_type() != data_type())
      return false;
    auto otherCasted = static_cast<const TupleType &>(other);
    return GTListEquality()(otherCasted.sub_types, sub_types);
  }

  virtual bool operator<(const GenericType &other) const {
    if (other.data_type() != data_type())
      return data_type() < other.data_type();
    auto otherCasted = static_cast<const TupleType &>(other);
    return GTListEquality()(otherCasted.sub_types, sub_types);
  };

  virtual llvm::Type *llvm_type() const {
    std::vector<llvm::Type *> type_list;
    for (auto i : sub_types) {
      type_list.push_back(i->llvm_type());
    }

    return llvm::StructType::get(llvm::getGlobalContext(), type_list);
  }

  virtual GenericValue create(llvm::Value *val) const {
    return GenericValue(std::make_shared<TupleType>(sub_types), val);
  }
};
