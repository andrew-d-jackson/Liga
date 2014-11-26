#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"

class MallocFunc {
public:
  llvm::Function *fn;
  llvm::Function *free_fn;

  MallocFunc(llvm::Module *m) {
    auto int_ptr = llvm::PointerType::get(
        llvm::IntegerType::getInt32Ty(llvm::getGlobalContext()), 0);
    auto int_ty = llvm::IntegerType::getInt32Ty(llvm::getGlobalContext());
    auto fn_ty = llvm::FunctionType::get(int_ptr, {int_ty}, false);
    fn = llvm::Function::Create(fn_ty, llvm::Function::ExternalLinkage,
                                "malloc", m);
    auto free_fn_ty = llvm::FunctionType::get(
        llvm::Type::getVoidTy(llvm::getGlobalContext()), {int_ptr}, false);
    free_fn = llvm::Function::Create(
        free_fn_ty, llvm::Function::ExternalLinkage, "free", m);
  }

  llvm::Value *raw_call(llvm::IRBuilder<> &builder, llvm::Value *size) {
    return builder.CreateCall(fn, size);
  }

  llvm::Value *raw_call(llvm::IRBuilder<> &builder, llvm::Value *size,
	  llvm::Type *t) {
	  auto c = raw_call(builder, size);
	  auto pty = llvm::PointerType::get(t, 0);
	  return builder.CreatePointerCast(c, pty);
  }

  llvm::Value *sized_call(Enviroment &env, llvm::IRBuilder<> &builder, llvm::Value *size,
	  llvm::Type *t) {
	  auto t_size = get_size(env, t);
	  auto new_size = builder.CreateMul(builder.getInt32(t_size), size);
	  auto c = raw_call(builder, new_size);
	  auto pty = llvm::PointerType::get(t, 0);
	  return builder.CreatePointerCast(c, pty);
  }

  llvm::Value *to_ptr(Enviroment env, llvm::IRBuilder<> &builder,
	  llvm::Value *val) {
	  auto val_size =
		  env.module->getDataLayout()->getTypeStoreSize(val->getType());
	  auto ret = raw_call(builder, builder.getInt32(val_size));
	  builder.CreateStore(val, ret, false);
	  return ret;
  }

  void free(llvm::IRBuilder<> &builder, llvm::Value *ptr) {
    auto cp = builder.CreatePointerCast(
        ptr, llvm::PointerType::get(
                 llvm::IntegerType::getInt32Ty(llvm::getGlobalContext()), 0));
    builder.CreateCall(free_fn, cp);
  }

  std::size_t get_size(Enviroment &env, llvm::Type *type) {
    if (type->isPointerTy()) {
		return 8;
		//return env.module->getDataLayout()->getPointerSize(0);
    } else if (type->isStructTy()) {
      std::size_t total = 0;
      for (auto it = type->subtype_begin(); it != type->subtype_end(); it++) {
        total += get_size(env, *it);
      }
      return total;
    } else {
      return env.module->getDataLayout()->getTypeStoreSize(type);
    }
  }

  llvm::Value *to_ptr(Enviroment &env, llvm::IRBuilder<> &builder,
                      std::vector<GenericValue> vals) {
   // std::size_t val_size = env.module->getDataLayout()->getTypeStoreSize(
     //                          vals.at(0).value->getType()) *
       //                    vals.size();

	  std::size_t val_size = get_size(env, vals.at(0).value->getType()) * vals.size();


    auto ret = raw_call(builder, builder.getInt32(val_size),
                        vals.at(0).type->llvm_type());

    for (int i = 0; i < vals.size(); i++) {
      auto g = builder.CreateGEP(ret, builder.getInt32(i));
      builder.CreateStore(vals.at(i).value, g, false);
    }

    return ret;
  }
};
