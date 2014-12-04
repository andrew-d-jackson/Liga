#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "Function.hpp"
#include "RefrenceCounter.hpp"

class VectorType : public GenericType {
public:
  GTPtr sub_type;

  VectorType(GTPtr sub_type) : sub_type(sub_type) {}
  virtual DataType data_type() const { return DataType::Vector; }
  virtual bool operator==(const GenericType &other) const {
    if (other.data_type() != data_type())
      return false;
    auto otherCasted = static_cast<const VectorType &>(other);
    return (*otherCasted.sub_type == *sub_type);
  }
  virtual bool operator<(const GenericType &other) const {
    if (other.data_type() != data_type())
      return data_type() < other.data_type();
    auto otherCasted = static_cast<const VectorType &>(other);
    return (*otherCasted.sub_type < *sub_type);
  };
  virtual llvm::Type *llvm_type() const {
    return llvm::StructType::get(
        llvm::getGlobalContext(),
        std::vector<llvm::Type *>(
            {IntegerType().llvm_type(), rc_type(sub_type->llvm_type())}));
  }
  virtual GenericValue create(llvm::Value *val) const {
    return GenericValue(std::make_shared<VectorType>(sub_type), val);
  }

  virtual GenericValue copy(Enviroment &env, llvm::IRBuilder<> &builder,
                            GenericValue val) const {
    auto rc_comp = builder.CreateExtractValue(val.value, {1});
    incriment_rc(builder, rc_comp);
    return val;
  };

  void for_each_element(Enviroment &env, llvm::IRBuilder<> &builder,
                        GenericValue vec,
                        std::function<void(GenericValue)> fn) const {

    auto vec_size = builder.CreateExtractValue(vec.value, {0});
    auto vec_rc = builder.CreateExtractValue(vec.value, {1});
    auto vec_arr = builder.CreateExtractValue(vec_rc, {1});

    create_loop(env, builder, vec_size, [&](GenericValue v) {
      auto element_ptr = builder.CreateGEP(vec_arr, {v.value});
      auto element = builder.CreateLoad(element_ptr);
      auto element_val = GenericValue{sub_type, element};
      fn(element_val);
    });
  }

  llvm::Function *destructor(Enviroment &env, llvm::IRBuilder<> &builder,
                             GenericValue val) const {
    static std::map<GTPtr, llvm::Function *> fn_map;
    if (fn_map.find(val.type) == fn_map.end()) {
      auto fn_ty = llvm::FunctionType::get(
          llvm::Type::getVoidTy(llvm::getGlobalContext()),
          {val.type->llvm_type()}, false);
      auto fn = llvm::Function::Create(fn_ty, llvm::Function::ExternalLinkage,
                                       "destruct", env.module);
      auto entry =
          llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", fn);
      auto build = llvm::IRBuilder<>(entry);
      for_each_element(env, build, val.type->create(fn->arg_begin()),
                       [&](GenericValue v) { v.type->destroy(env, build, v); });
      auto vec_gc_comp = build.CreateExtractValue(fn->arg_begin(), {1});
      destruct_rc(env, build, vec_gc_comp);
      build.CreateRetVoid();
      fn_map[val.type] = fn;
    }
    return fn_map[val.type];
  }

  virtual void destroy(Enviroment &env, llvm::IRBuilder<> &builder,
                       GenericValue val) const {
    auto des = destructor(env, builder, val);
    builder.CreateCall(des, {val.value});
  };
};
