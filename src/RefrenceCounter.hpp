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

extern llvm::Value *to_rc(Enviroment &env, llvm::IRBuilder<> &builder,
	llvm::Value *ptr);
extern void incriment_rc(llvm::IRBuilder<> &builder, llvm::Value *rc_seg);
extern void destruct_rc(Enviroment &env, llvm::IRBuilder<> &builder,
	llvm::Value *rc_val);
extern llvm::StructType *rc_type(llvm::Type *sub_type);