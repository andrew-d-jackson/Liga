#include "Tuple.hpp"

DataType TupleType::data_type() const { return DataType::Tuple; }

bool TupleType::operator==(const GenericType &other) const {
  if (other.data_type() != data_type())
    return false;
  auto otherCasted = static_cast<const TupleType &>(other);
  return GTListEquality()(otherCasted.sub_types, sub_types);
}

bool TupleType::operator<(const GenericType &other) const {
  if (other.data_type() != data_type())
    return data_type() < other.data_type();
  auto otherCasted = static_cast<const TupleType &>(other);
  return GTListEquality()(otherCasted.sub_types, sub_types);
};

llvm::Type *TupleType::llvm_type() const {
  std::vector<llvm::Type *> type_list;
  for (auto i : sub_types) {
    type_list.push_back(i->llvm_type());
  }

  return llvm::StructType::get(llvm::getGlobalContext(), type_list);
}

GenericValue TupleType::create(llvm::Value *val) const {
  return GenericValue(std::make_shared<TupleType>(sub_types), val);
}
