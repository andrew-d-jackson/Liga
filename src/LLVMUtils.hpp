#pragma once
#include "LLVM.hpp"

#include <memory>
#include <functional>

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "Types/Integer.hpp"

extern std::string string_representation(llvm::Module &mod);

extern void create_loop(Enviroment &env, llvm::IRBuilder<> &builder,
	llvm::Value *times, std::function<void(GenericValue)> fn);