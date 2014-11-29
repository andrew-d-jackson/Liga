#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "Function.hpp"
#include "Types.hpp"

#include <functional>

class DefineMacro : public Macro {
public:
  virtual GenericValue call(Enviroment &env, llvm::IRBuilder<> &builder,
                            std::vector<std::shared_ptr<ASTNode>> args) {
    auto symbol = static_cast<ASTSymbol *>(args.at(0).get())->val;
    env.value_map[symbol] = args.at(1)->to_value(env, builder);
    return GenericValue(std::make_shared<BooleanType>(), builder.getTrue());
  }
  virtual GTPtr return_type(Enviroment &env,
                            std::vector<std::shared_ptr<ASTNode>> args) {
    return std::make_shared<BooleanType>();
  }
};

class IfMacro : public Macro {
public:
  virtual GenericValue call(Enviroment &env, llvm::IRBuilder<> &builder,
                            ASTList args) {
    auto if_true_block = llvm::BasicBlock::Create(
        llvm::getGlobalContext(), "if_true",
        (llvm::Function *)builder.GetInsertPoint()->getParent());
    auto if_false_block = llvm::BasicBlock::Create(
        llvm::getGlobalContext(), "if_false",
        (llvm::Function *)builder.GetInsertPoint()->getParent());
    auto if_join_block = llvm::BasicBlock::Create(
        llvm::getGlobalContext(), "if_join",
        (llvm::Function *)builder.GetInsertPoint()->getParent());

    auto evaluated_condition = args.at(0)->to_value(env, builder);
    builder.CreateCondBr(evaluated_condition.value, if_true_block,
                         if_false_block);


    builder.SetInsertPoint(if_true_block);
	Scope new_scope_true;
	auto sub_env_true = env.with_new_scope(new_scope_true);
	auto evaluated_true_branch = args.at(1)->to_value(sub_env_true, builder);
	new_scope_true.destroy_all_but(env, builder, evaluated_true_branch);
    builder.CreateBr(if_join_block);

	builder.SetInsertPoint(if_false_block);
	Scope new_scope_false;
	auto sub_env_false = env.with_new_scope(new_scope_false);
	auto evaluated_false_branch = args.at(2)->to_value(sub_env_false, builder);
	new_scope_false.destroy_all_but(env, builder, evaluated_false_branch);
    builder.CreateBr(if_join_block);

    builder.SetInsertPoint(if_join_block);
    auto joined =
        builder.CreatePHI(evaluated_true_branch.type->llvm_type(), 2, "joined");
    joined->addIncoming(evaluated_true_branch.value, if_true_block);
    joined->addIncoming(evaluated_false_branch.value, if_false_block);

    auto ret = GenericValue(evaluated_true_branch.type, joined);
    env.scope.add(ret);
    return ret;
  }

  virtual GTPtr return_type(Enviroment &env, ASTList args) {
    return args.at(1)->return_type(env);
  }
};

class PrintFunc : public Function {
public:
  void create_mapping(Enviroment &env, GTPtr ty) {
    auto fn_ty =
        llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()),
                                {ty->llvm_type()}, false);
    auto fn = llvm::Function::Create(fn_ty, llvm::Function::ExternalLinkage,
                                     "print", env.module);
    fn_map.insert({ty, fn});
  }

  void create_vector_mapping(Enviroment &env, GTPtr ty) {
    auto vec_ty = static_cast<VectorType *>(ty.get());
    auto fn_ty =
        llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()),
                                {ty->llvm_type()}, false);

    auto fn = llvm::Function::Create(fn_ty, llvm::Function::ExternalLinkage,
                                     "print_vec", env.module);

    auto entry =
        llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", fn);
    llvm::IRBuilder<> build(entry);
    auto arg = fn->arg_begin();
    arg->setName("vec");
    auto vec_size = build.CreateExtractValue(arg, {0}, "vec_size");
    auto vec_rc = build.CreateExtractValue(arg, {1}, "vec_rc");
    auto vec_arr = build.CreateExtractValue(vec_rc, {1}, "vec_arr");

    create_loop(env, build, vec_size, [&](GenericValue v) {
      auto c = build.CreateGEP(vec_arr, v.value);
      auto cl = build.CreateLoad(c);
      call_without_endl(env, build, {GenericValue(vec_ty->sub_type, cl)});
    });

    build.CreateRetVoid();

    fn_map[ty] = fn;
  }

  void call_without_endl(Enviroment &env, llvm::IRBuilder<> &builder,
                         std::vector<GenericValue> vals) {
    auto not_found = fn_map.find(vals.at(0).type) == fn_map.end();
    if (not_found) {
      for (auto i : fn_map) {
        i.first->llvm_type()->dump();
      }
      create_mapping(env, vals.at(0).type);
    }
    auto call_fn = fn_map[vals.at(0).type];
    builder.CreateCall(call_fn, vals.at(0).value);
  }

  GenericValue call(Enviroment &env, llvm::IRBuilder<> &builder,
                    std::vector<GenericValue> vals) {
    call_without_endl(env, builder, vals);
    builder.CreateCall(fn_map[std::make_shared<CharType>()],
                       builder.getInt8('\n'));
	return VoidType().create();
  }

  virtual GTPtr return_type(Enviroment &env, std::vector<GTPtr> args) {
    return std::make_shared<VoidType>();
  }

  std::map<GTPtr, llvm::Function *, GTPtrComparison> fn_map;
  llvm::Function *fn;
};

class AtFunc : public Function {
public:
  GenericValue call(Enviroment &env, llvm::IRBuilder<> &builder,
                    std::vector<GenericValue> vals) {
    auto idx = vals.at(0);
    auto vec = vals.at(1);

    auto vec_arr_rc = builder.CreateExtractValue(vec.value, {1});
    auto vec_arr_ptr = builder.CreateExtractValue(vec_arr_rc, {1});
    auto ret_val_ptr = builder.CreateGEP(vec_arr_ptr, idx.value);
    auto ret_val = builder.CreateLoad(ret_val_ptr);

    auto vec_type = static_cast<VectorType *>(vec.type.get());
    auto ret = GenericValue{vec_type->sub_type, ret_val};
    return ret;
  }
  virtual GTPtr return_type(Enviroment &env, std::vector<GTPtr> args) {
    return static_cast<VectorType *>(args.at(1).get())->sub_type;
  }
};

class AddFunc : public Function {
public:
	GenericValue call(Enviroment &env, llvm::IRBuilder<> &builder,
		std::vector<GenericValue> vals) {
		auto one = vals.at(0);
		auto two = vals.at(1);

		if (one.type->data_type() == DataType::Integer &&
			two.type->data_type() == DataType::Integer) {
			return GenericValue(std::make_shared<IntegerType>(),
				builder.CreateAdd(one.value, two.value));
		}
		auto first = one.type->data_type() == DataType::Integer
			? builder.CreateSIToFP(one.value, FloatType().llvm_type())
			: one.value;
		auto second = two.type->data_type() == DataType::Integer
			? builder.CreateSIToFP(two.value, FloatType().llvm_type())
			: two.value;

		return GenericValue(std::make_shared<FloatType>(),
			builder.CreateFAdd(first, second));
	}
	virtual GTPtr return_type(Enviroment &env, std::vector<GTPtr> args) {
		if (args.at(0)->data_type() == DataType::Float ||
			args.at(1)->data_type() == DataType::Float)
			return std::make_shared<FloatType>();
		return std::make_shared<IntegerType>();
	}
};

class ComparisonFunc : public Function {
public:
	virtual GTPtr return_type(Enviroment &env, std::vector<GTPtr> args) {
		return std::make_shared<BooleanType>();
	}

	GenericValue call(Enviroment &env, llvm::IRBuilder<> &builder,
		std::vector<GenericValue> vals) {
		auto one = vals.at(0);
		auto two = vals.at(1);

		if (one.type->data_type() == DataType::Integer &&
			two.type->data_type() == DataType::Integer) {
			return GenericValue(std::make_shared<BooleanType>(),
				int_cmp(builder, one.value, two.value));
		}
		auto first = one.type->data_type() == DataType::Integer
			? builder.CreateSIToFP(one.value, FloatType().llvm_type())
			: one.value;
		auto second = two.type->data_type() == DataType::Integer
			? builder.CreateSIToFP(two.value, FloatType().llvm_type())
			: two.value;

		return GenericValue(std::make_shared<BooleanType>(),
			float_cmp(builder, first, second));
	}

	virtual llvm::Value* int_cmp(llvm::IRBuilder<> &builder, llvm::Value* a, llvm::Value* b) = 0;
	virtual llvm::Value* float_cmp(llvm::IRBuilder<> &builder, llvm::Value* a, llvm::Value* b) = 0;
};

class EqualityFunc : public ComparisonFunc {
public:
	virtual llvm::Value* int_cmp(llvm::IRBuilder<> &builder, llvm::Value* a, llvm::Value* b) {
		return builder.CreateICmpEQ(a, b);
	}
	virtual llvm::Value* float_cmp(llvm::IRBuilder<> &builder, llvm::Value* a, llvm::Value* b) {
		return builder.CreateFCmpOEQ(a, b);
	}
};

class LessThanFunc : public ComparisonFunc {
public:
	virtual llvm::Value* int_cmp(llvm::IRBuilder<> &builder, llvm::Value* a, llvm::Value* b) {
		return builder.CreateICmpSLT(a, b);
	}
	virtual llvm::Value* float_cmp(llvm::IRBuilder<> &builder, llvm::Value* a, llvm::Value* b) {
		return builder.CreateFCmpOLT(a, b);
	}
};

class GreaterThanFunc : public ComparisonFunc {
public:
	virtual llvm::Value* int_cmp(llvm::IRBuilder<> &builder, llvm::Value* a, llvm::Value* b) {
		return builder.CreateICmpSGT(a, b);
	}
	virtual llvm::Value* float_cmp(llvm::IRBuilder<> &builder, llvm::Value* a, llvm::Value* b) {
		return builder.CreateFCmpOGT(a, b);
	}
};

class AppendFunc : public Function {
public:
  GenericValue call(Enviroment &env, llvm::IRBuilder<> &builder,
                    std::vector<GenericValue> vals) {
    auto orig_vec = vals.at(0).value;
    auto item = vals.at(1).value;
    auto orig_vec_ty = static_cast<VectorType *>(vals.at(0).type.get());

    auto vec_arr_rc = builder.CreateExtractValue(orig_vec, {1});
    auto vec_arr_ptr = builder.CreateExtractValue(vec_arr_rc, {1});
    auto orig_size = builder.CreateExtractValue(orig_vec, {0});
    auto new_size = builder.CreateAdd(orig_size, builder.getInt32(1));
    auto struct_type =
        static_cast<llvm::StructType *>(vals.at(0).type->llvm_type());

    auto new_arr_ptr = env.malloc_fn.sized_call(
        env, builder, new_size, orig_vec_ty->sub_type->llvm_type());

    create_loop(env, builder, orig_size, [&](GenericValue v) {
      auto new_gep_ptr = builder.CreateGEP(new_arr_ptr, v.value);
      auto old_gep_ptr = builder.CreateGEP(vec_arr_ptr, v.value);
      auto old_val = builder.CreateLoad(old_gep_ptr);
      old_val->dump();
      builder.CreateStore(old_val, new_gep_ptr);
    });
    auto new_gep_ptr = builder.CreateGEP(new_arr_ptr, orig_size);
    builder.CreateStore(item, new_gep_ptr);

    auto rc = to_rc(env, builder, new_arr_ptr);
    auto ret_base = llvm::ConstantStruct::get(
        struct_type, std::vector<llvm::Constant *>(
                         {builder.getInt32(0),
                          llvm::Constant::getNullValue(rc->getType())}));

    auto ret_llvm = builder.CreateInsertValue(ret_base, new_size, {0});
    ret_llvm = builder.CreateInsertValue(ret_llvm, rc, {1});

    auto ret = vals.at(0).type->create(ret_llvm);
    env.scope.add(ret);
    return ret;
  }
  virtual GTPtr return_type(Enviroment &env, std::vector<GTPtr> args) {
    return args.at(1);
  }
};

class LambdaBase : public Function {
public:
  std::map<std::vector<GTPtr>, llvm::Function *> fn_map;
  std::map<std::vector<GTPtr>, GTPtr> return_type_map;
  ASTProcess proc;
  std::vector<std::string> arg_names;
  Enviroment internal_env;

  LambdaBase(ASTProcess proc, std::vector<std::string> arg_names,
             Enviroment internal_env)
      : proc(proc), arg_names(arg_names), internal_env(internal_env) {}

  void create_fn(std::vector<GTPtr> types) {
    auto temp_env = internal_env;
    for (int i = 0; i < types.size(); i++) {
      temp_env.value_map[arg_names.at(i)] = types.at(i)->create(nullptr);
    }
    auto return_type = proc.return_type(temp_env);
    std::vector<llvm::Type *> llvm_types;
    for (const auto &i : types)
      llvm_types.push_back(i->llvm_type());

    auto fn_ty =
        llvm::FunctionType::get(return_type->llvm_type(), llvm_types, false);
    auto fn = llvm::Function::Create(fn_ty, llvm::Function::ExternalLinkage,
                                     "custom_function", internal_env.module);

    auto entry =
        llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", fn);
    auto build = llvm::IRBuilder<>(entry);
    Enviroment new_env = internal_env;

    auto it = fn->arg_begin();
    for (int i = 0; i < types.size(); i++) {
      new_env.value_map[arg_names.at(i)] = types.at(i)->create(it++);
    }

    build.CreateRet(proc.to_value(new_env, build).value);
    fn_map.insert({types, fn});
    return_type_map.insert({types, return_type});
  };

  GenericValue call(Enviroment &env, llvm::IRBuilder<> &builder,
                    std::vector<GenericValue> vals) {

    std::vector<GTPtr> types;
    for (const auto &i : vals)
      types.push_back(i.type);

    auto not_found = fn_map.find(types) == fn_map.end();
    if (not_found)
      create_fn(types);

    auto call_fn = fn_map[types];
    std::vector<llvm::Value *> raw_args;
    for (const auto &i : vals)
      raw_args.push_back(i.value);
    auto return_value = builder.CreateCall(call_fn, raw_args);
    return return_type_map[types]->create(return_value);
  }

  virtual GTPtr return_type(Enviroment &env, std::vector<GTPtr> args) {
    auto not_found = fn_map.find(args) == fn_map.end();
    if (not_found)
      create_fn(args);

    return return_type_map[args];
  }
};

class LambdaMacro : public Macro {
public:
  std::map<GTPtr, LambdaBase> created;

  virtual GenericValue call(Enviroment &env, llvm::IRBuilder<> &builder,
                            std::vector<std::shared_ptr<ASTNode>> args) {
    auto proc_ast = static_cast<ASTProcess *>(args.at(1).get());
    auto arg_ast = static_cast<ASTVector *>(args.at(0).get());

    auto arg_list = std::vector<std::string>();
    for (auto i : arg_ast->val)
      arg_list.push_back(static_cast<ASTSymbol *>(i.get())->val);

    return make_func(env,
                     std::make_shared<LambdaBase>(*proc_ast, arg_list, env));
  }

  virtual GTPtr return_type(Enviroment &env,
                            std::vector<std::shared_ptr<ASTNode>> args) {
    return std::make_shared<FunctionType>(
        [](std::vector<GTPtr> a) { return a.at(0); });
  }
};
