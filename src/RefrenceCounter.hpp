#pragma once
#include "LLVM.hpp"
#include "LLVMUtils.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "Malloc.hpp"

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <functional>
#include <exception>

llvm::Value *to_rc(Enviroment &env, llvm::IRBuilder<> &builder,
                   llvm::Value *ptr) {
  auto int_ty = llvm::IntegerType::getInt32Ty(llvm::getGlobalContext());
  auto int_ptr_ty = llvm::PointerType::get(int_ty, 0);
  auto type = llvm::StructType::get(
      llvm::getGlobalContext(),
      std::vector<llvm::Type *>({int_ptr_ty, ptr->getType()}));
  auto structure = llvm::ConstantStruct::get(
      type, std::vector<llvm::Constant *>(
                {llvm::Constant::getNullValue(int_ptr_ty),
                 llvm::Constant::getNullValue(ptr->getType())}));

  auto count_ptr = env.malloc_fn->to_ptr(env, builder, builder.getInt32(1));
  auto ret = builder.CreateInsertValue(structure, count_ptr, {0});
  ret = builder.CreateInsertValue(ret, ptr, {1});

  return ret;
}

void incriment_rc(llvm::IRBuilder<> &builder, llvm::Value *rc_seg) {
  auto rc_ptr = builder.CreateExtractValue(rc_seg, {0});
  auto val = builder.CreateLoad(rc_ptr);
  auto new_val = builder.CreateAdd(val, builder.getInt32(1));
  builder.CreateStore(new_val, rc_ptr);
}

void destruct_rc(Enviroment &env, llvm::IRBuilder<> &builder,
                 llvm::Value *rc_val) {
  static std::map<llvm::Type *, llvm::Function *> fn_map;

    auto rc_val_str_type = rc_val->getType();
    auto rc_val_ptr = builder.CreateExtractValue(rc_val, {1});
  auto rc_val_ty = rc_val_ptr->getType();

  if (fn_map.find(rc_val_ty) == fn_map.end()) {
    auto fn_ty =
        llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()),
                                {rc_val_str_type}, false);
    auto fn = llvm::Function::Create(fn_ty, llvm::Function::ExternalLinkage,
                                     "rc_destroy", env.module);
    auto fn_entry =
        llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", fn);
    auto fn_builder = llvm::IRBuilder<>(fn_entry);
    auto arg = fn->arg_begin();
    arg->setName("in");

    auto count_ptr = fn_builder.CreateExtractValue(arg, {0});
    auto count = fn_builder.CreateLoad(count_ptr);
    auto value_ptr = fn_builder.CreateExtractValue(arg, {1});
    auto new_count = fn_builder.CreateSub(count, fn_builder.getInt32(1));

    auto no_ref_block = llvm::BasicBlock::Create(llvm::getGlobalContext(),
                                                 "if_no_refrences", fn);
    auto some_ref_block = llvm::BasicBlock::Create(llvm::getGlobalContext(),
                                                   "if_some_refrences", fn);
    auto join_block =
        llvm::BasicBlock::Create(llvm::getGlobalContext(), "join_if", fn);

    auto if_no_refrences =
        fn_builder.CreateICmpEQ(new_count, fn_builder.getInt32(0));
    auto if_branch =
        fn_builder.CreateCondBr(if_no_refrences, no_ref_block, some_ref_block);

    fn_builder.SetInsertPoint(some_ref_block);
    fn_builder.CreateStore(new_count, count_ptr);
    fn_builder.CreateBr(join_block);

    fn_builder.SetInsertPoint(no_ref_block);
    env.malloc_fn->free(fn_builder, count_ptr);
    env.malloc_fn->free(fn_builder, value_ptr);
    fn_builder.CreateBr(join_block);

    fn_builder.SetInsertPoint(join_block);
    fn_builder.CreateRetVoid();

    fn_map[rc_val_ty] = fn;
  }
  builder.CreateCall(fn_map[rc_val_ty], {rc_val});
}

llvm::StructType *rc_type(llvm::Type *sub_type) {
  auto int_ty = llvm::IntegerType::getInt32Ty(llvm::getGlobalContext());
  auto int_ptr_ty = llvm::PointerType::get(int_ty, 0);
  auto sub_ptr_ty = llvm::PointerType::get(sub_type, 0);
  return llvm::StructType::get(
      llvm::getGlobalContext(),
      std::vector<llvm::Type *>({int_ptr_ty, sub_ptr_ty}));
}
