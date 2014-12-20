#include "Tuple.hpp"

#include "Types/Tuple.hpp"

DataType ASTTuple::type() const { return DataType::Tuple; }

GTPtr ASTTuple::return_type(Enviroment &env) const {
  std::vector<GTPtr> type_list;
  for (auto i : val) {
    type_list.push_back(i->return_type(env));
  }

  return std::make_shared<TupleType>(type_list);
}

GenericValue ASTTuple::to_value(Enviroment &env, llvm::IRBuilder<> &builder) {
  std::vector<GenericValue> vals;
  std::vector<GTPtr> val_types;
  std::vector<llvm::Type *> val_llvm_types;
  for (auto i : val) {
    vals.push_back(i->to_value(env, builder));
    val_types.push_back(i->return_type(env));
    val_llvm_types.push_back(i->return_type(env)->llvm_type());
  }

  auto st_ty =
      llvm::StructType::get(llvm::getGlobalContext(), val_llvm_types, false);
  llvm::Value *st = llvm::Constant::getNullValue(st_ty);

  unsigned index = 0;
  for (auto i : vals) {
    st = builder.CreateInsertValue(st, i.value, {index});
    index++;
  }

  auto ret = TupleType(val_types).create(st);
  return ret;
}

std::string ASTTuple::as_string() const {
  std::string ret = "{ ";
  for (const auto &i : val) {
    ret += i->as_string();
    ret += ' ';
  }
  ret += "}";
  return ret;
}
