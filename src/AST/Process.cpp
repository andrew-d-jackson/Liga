#include "Process.hpp"

#include "Function.hpp"

DataType ASTProcess::type() const { return DataType::Process; }

bool ASTProcess::is_prefix_call(Enviroment &env, DataType ty) const {
  auto first_dt = val.at(0)->return_type(env)->data_type();
  if (first_dt == ty) {
    if (val.size() > 1)
      return true;
  }
  return false;
}

bool ASTProcess::is_infix_func_call(Enviroment &env) const {
  if (val.size() != 3)
    return false;
  if (val.at(0)->return_type(env)->data_type() == DataType::Function)
    return false;
  if (val.at(1)->return_type(env)->data_type() == DataType::Function)
    return true;
  return false;
}

std::vector<GTPtr> ASTProcess::get_value_types(Enviroment &env) const {
	std::vector<GTPtr> val_types;
	for (const auto &i : val) {
		val_types.push_back(i->return_type(env));
	}
	return val_types;
}

std::vector<GenericValue>
ASTProcess::generate_values(Enviroment &env, llvm::IRBuilder<> &builder) const {
  std::vector<GenericValue> values;
  for (const auto &i : val) {
    values.push_back(i->to_value(env, builder));
  }
  return values;
}

std::vector<GenericValue>
ASTProcess::generate_values_impurley(Enviroment &env, llvm::IRBuilder<> &builder) const {
	std::vector<GenericValue> values;
	for (const auto &i : val) {
		if (i->type() == DataType::Process){
			auto i_proc = static_cast<ASTProcess*>(i.get());
			values.push_back(i_proc->to_value(env, builder, false));
		} else {
			values.push_back(i->to_value(env, builder));
		}
	}
	return values;

}

GenericValue reduce(Enviroment &env, llvm::IRBuilder<> &builder, ASTProcess p) {
	auto ret_ty = p.return_type(env);
	auto temp_ty = llvm::FunctionType::get(
		ret_ty->llvm_type(), false);

	auto temp_fn = llvm::Function::Create(
		temp_ty, llvm::Function::ExternalLinkage, "temp", env.module);

	auto temp_entry = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", temp_fn);
	auto new_builder = llvm::IRBuilder<>(temp_entry);
	auto new_scope = Scope();
	auto new_env = env.with_new_scope(new_scope);
	auto ret = p.to_value(new_env, new_builder, true);
	new_env.scope.create_return(new_env, new_builder, ret);

	llvm::ExecutionEngine *EE = llvm::EngineBuilder(env.module).create();
	auto gv = EE->runFunction(temp_fn, {});
	auto ret_ast = ret_ty->create_ast(gv);
	return ret_ast->to_value(env, builder);
		
}


GenericValue ASTProcess::to_value(Enviroment &env, llvm::IRBuilder<> &builder, bool dont_evaluate_purely) {
	if (is_prefix_call(env, DataType::Macro)) {
		auto mac_args =
			std::vector<std::shared_ptr<ASTNode>>(val.begin() + 1, val.end());
		return val.at(0)
			->to_value(env, builder)
			.macro->call(env, builder, mac_args);
	}


	if (is_prefix_call(env, DataType::Function)) {
		if (is_pure(env) && !dont_evaluate_purely) {
			return reduce(env, builder, *this);
		}
		auto values = dont_evaluate_purely ? generate_values_impurley(env, builder) :
				generate_values(env, builder);
		auto fn_args = std::vector<GenericValue>(values.begin() + 1, values.end());
		return values.at(0).func->call(env, builder, fn_args);
	}

	auto values = dont_evaluate_purely ? generate_values_impurley(env, builder) :
			generate_values(env, builder);

	if (is_infix_func_call(env)) {
		std::vector<GenericValue> fn_args;
		fn_args.push_back(values.at(0));
		fn_args.push_back(values.at(2));
		return values.at(1).func->call(env, builder, fn_args);
	}

	return *(values.end() - 1);
}

GenericValue ASTProcess::to_value(Enviroment &env, llvm::IRBuilder<> &builder) {
	return to_value(env, builder, false);
};

GTPtr ASTProcess::return_type(Enviroment &env) const {

  if (is_prefix_call(env, DataType::Macro)) {
    auto args_list =
        std::vector<std::shared_ptr<ASTNode>>(val.begin() + 1, val.end());
    return static_cast<MacroType *>(val.at(0)->return_type(env).get())
        ->return_type(env, args_list);
  }

  auto val_types = get_value_types(env);

  if (is_prefix_call(env, DataType::Function)) {
    auto args_list = std::vector<GTPtr>(val_types.begin() + 1, val_types.end());
    return static_cast<FunctionType *>(val_types.at(0).get())
        ->return_type(env, args_list);
  }

  if (is_infix_func_call(env)) {
    std::vector<GTPtr> args_list;
    args_list.push_back(val_types.at(0));
    args_list.push_back(val_types.at(2));
    return static_cast<FunctionType *>(val_types.at(1).get())
        ->return_type(env, args_list);
  }

  return *(val_types.end() - 1);
}

std::string ASTProcess::as_string() const {
  std::string ret = "[ ";
  for (const auto &i : val) {
    ret += i->as_string();
    ret += ' ';
  }
  ret += "]";
  return ret;
}

bool ASTProcess::is_pure(Enviroment &env) const {
	for (auto &i : val) {
		if (!i->is_pure(env)) return false;
	}

	if (is_prefix_call(env, DataType::Macro)) {
		auto args_list =
			std::vector<std::shared_ptr<ASTNode>>(val.begin() + 1, val.end());
		return static_cast<MacroType *>(val.at(0)->return_type(env).get())
			->is_pure(env, args_list);
	}

	auto val_types = get_value_types(env);

	if (is_prefix_call(env, DataType::Function)) {
		auto args_list = std::vector<GTPtr>(val_types.begin() + 1, val_types.end());
		return static_cast<FunctionType *>(val_types.at(0).get())
			->is_pure(env, args_list);
	}

	if (is_infix_func_call(env)) {
		std::vector<GTPtr> args_list;
		args_list.push_back(val_types.at(0));
		args_list.push_back(val_types.at(2));
		return static_cast<FunctionType *>(val_types.at(1).get())
			->is_pure(env, args_list);
	}

	return true;
}
