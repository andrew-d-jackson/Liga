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

void for_each_vec(Enviroment &env, llvm::IRBuilder<> &builder, GenericValue vec,
                  std::function<void(GenericValue, GenericValue)> fn) {

  auto vec_size = builder.CreateExtractValue(vec.value, {0});
  auto vec_rc = builder.CreateExtractValue(vec.value, {1});
  auto vec_arr = builder.CreateExtractValue(vec_rc, {1});

  create_loop(env, builder, vec_size, [&](GenericValue v) {
    auto element_ptr = builder.CreateGEP(vec_arr, {v.value});
    auto element = builder.CreateLoad(element_ptr);
    auto sub_type = static_cast<VectorType *>(vec.type.get())->sub_type;
    auto element_val = GenericValue{sub_type, element};
    fn(element_val, v);
  });
}

GenericValue create_vec(Enviroment &env, llvm::IRBuilder<> &builder,
                        GTPtr vec_type, llvm::Value *size, llvm::Value *arr) {
  auto rc = to_rc(env, builder, arr);

  auto st_ty = llvm::StructType::get(
      llvm::getGlobalContext(),
      std::vector<llvm::Type *>({IntegerType().llvm_type(), rc->getType()}));

  auto ret_base = llvm::Constant::getNullValue(st_ty);
  auto ret = builder.CreateInsertValue(ret_base, size, {0});
  ret = builder.CreateInsertValue(ret, rc, {1});

  auto return_generic = vec_type->create(ret);
  env.scope.add(return_generic);
  return return_generic;
}

llvm::Value *get_vec_arr_ptr(llvm::IRBuilder<> &builder,
                             GenericValue original) {
  auto original_gc_ptr = builder.CreateExtractValue(original.value, {1});
  auto original_arr_ptr = builder.CreateExtractValue(original_gc_ptr, {1});
  return original_arr_ptr;
}

llvm::Value *get_vec_size(llvm::IRBuilder<> &builder, GenericValue original) {
  auto original_size = builder.CreateExtractValue(original.value, {0});
  return original_size;
}

GTPtr get_vec_sub_type(llvm::IRBuilder<> &builder, GenericValue original) {
  auto original_type = static_cast<VectorType *>(original.type.get());
  auto original_subtype = original_type->sub_type;
  return original_subtype;
}

GenericValue copy_vec(Enviroment &env, llvm::IRBuilder<> &builder,
                      GenericValue original) {
  auto original_subtype = get_vec_sub_type(builder, original);
  auto original_size = get_vec_size(builder, original);

  auto new_arr_ptr = env.malloc_fn.sized_call(env, builder, original_size,
                                              original_subtype->llvm_type());

  for_each_vec(env, builder, original, [&](GenericValue v, GenericValue i) {
    auto elm_ptr = builder.CreateGEP(new_arr_ptr, i.value);
    builder.CreateStore(v.value, elm_ptr);
  });

  auto ret =
      create_vec(env, builder, original.type, original_size, new_arr_ptr);
  return ret;
}