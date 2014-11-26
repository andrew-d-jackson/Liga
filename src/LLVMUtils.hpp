#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "Types/Integer.hpp"

std::string string_representation(llvm::Module &mod) {
  std::string mod_str;
  llvm::raw_string_ostream rso(mod_str);
  mod.print(rso, nullptr);
  return mod_str;
}

void create_loop(Enviroment &env, llvm::IRBuilder<> &builder,
                 llvm::Value *times, std::function<void(GenericValue)> fn) {
  auto loop = llvm::BasicBlock::Create(llvm::getGlobalContext(), "loop",
                                       builder.GetInsertBlock()->getParent());
  auto end_loop =
      llvm::BasicBlock::Create(llvm::getGlobalContext(), "end_loop",
                               builder.GetInsertBlock()->getParent());

  auto index_ptr = builder.CreateAlloca(builder.getInt32Ty());
  builder.CreateStore(builder.getInt32(0), index_ptr);
  builder.CreateBr(loop);

  builder.SetInsertPoint(loop);
  auto index = builder.CreateLoad(index_ptr);
  auto new_index = builder.CreateAdd(index, builder.getInt32(1));
  builder.CreateStore(new_index, index_ptr);

  fn(GenericValue(std::make_shared<IntegerType>(), index));
  auto if_stmt = builder.CreateICmpEQ(times, new_index);
  auto if_branch = builder.CreateCondBr(if_stmt, end_loop, loop);

  builder.SetInsertPoint(end_loop);
}