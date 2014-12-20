#pragma once
#include "LLVM.hpp"

#include "LLVMUtils.hpp"
#include "Malloc.hpp"
#include "GenericValue.hpp"
#include "RefrenceCounter.hpp"
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

class AtTupleFunc : public Macro {
public:
  virtual GenericValue call(Enviroment &env, llvm::IRBuilder<> &builder,
                            std::vector<std::shared_ptr<ASTNode>> args) {
    auto idx = args.at(0);
    auto vec = args.at(1);

    auto index = static_cast<ASTInteger *>(idx.get());
    auto tuple = vec->to_value(env, builder);
    auto tuple_ty = static_cast<TupleType *>(tuple.type.get());
    auto ret_ty = tuple_ty->sub_types.at(index->val);

    auto ret_val = builder.CreateExtractValue(tuple.value, index->val);
    return ret_ty->create(ret_val);
  }

  virtual GTPtr return_type(Enviroment &env,
                            std::vector<std::shared_ptr<ASTNode>> args) {
    auto idx = args.at(0);
    auto vec = args.at(1);

    auto index = static_cast<ASTInteger *>(idx.get());
    auto tuple = vec->return_type(env);
    auto tuple_ty = static_cast<TupleType *>(tuple.get());
    auto ret_ty = tuple_ty->sub_types.at(index->val);
    return ret_ty;
  }
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

class BooleanNotFunc : public Function {
public:
  GenericValue call(Enviroment &env, llvm::IRBuilder<> &builder,
                    std::vector<GenericValue> vals) {
    auto arg = vals.at(0).value;
    auto ret_val = builder.CreateNot(arg);
    return BooleanType().create(ret_val);
  }
  virtual GTPtr return_type(Enviroment &env, std::vector<GTPtr> args) {
    return std::make_shared<BooleanType>();
  }
};

class BooleanBaseFunc : public Function {
public:
  virtual llvm::Value *eval(llvm::IRBuilder<> &builder, llvm::Value *a,
                            llvm::Value *b) const = 0;

  GenericValue call(Enviroment &env, llvm::IRBuilder<> &builder,
                    std::vector<GenericValue> vals) {
    auto arg1 = vals.at(0).value;
    auto arg2 = vals.at(1).value;

    auto ret_val = eval(builder, arg1, arg2);

    return BooleanType().create(ret_val);
  }
  virtual GTPtr return_type(Enviroment &env, std::vector<GTPtr> args) {
    return std::make_shared<BooleanType>();
  }
};

class BooleanAndFunc : public BooleanBaseFunc {
public:
  virtual llvm::Value *eval(llvm::IRBuilder<> &builder, llvm::Value *a,
                            llvm::Value *b) const {
    return builder.CreateAnd(a, b);
  }
};

class BooleanOrFunc : public BooleanBaseFunc {
public:
  virtual llvm::Value *eval(llvm::IRBuilder<> &builder, llvm::Value *a,
                            llvm::Value *b) const {
    return builder.CreateOr(a, b);
  }
};

class BooleanXorFunc : public BooleanBaseFunc {
public:
  virtual llvm::Value *eval(llvm::IRBuilder<> &builder, llvm::Value *a,
                            llvm::Value *b) const {
    return builder.CreateXor(a, b);
  }
};

class MathFunc : public Function {
public:
  GenericValue call(Enviroment &env, llvm::IRBuilder<> &builder,
                    std::vector<GenericValue> vals) {
    auto one = vals.at(0);
    auto two = vals.at(1);

    if (one.type->data_type() == DataType::Integer &&
        two.type->data_type() == DataType::Integer) {
      return GenericValue(std::make_shared<IntegerType>(),
                          int_eval(builder, one.value, two.value));
    }
    auto first = to_float(builder, one);
    auto second = to_float(builder, two);

    return GenericValue(std::make_shared<FloatType>(),
                        float_eval(builder, first.value, second.value));
  }

  virtual GTPtr return_type(Enviroment &env, std::vector<GTPtr> args) {
    if (args.at(0)->data_type() == DataType::Float ||
        args.at(1)->data_type() == DataType::Float)
      return std::make_shared<FloatType>();
    return std::make_shared<IntegerType>();
  }

  virtual llvm::Value *int_eval(llvm::IRBuilder<> &builder, llvm::Value *a,
                                llvm::Value *b) = 0;
  virtual llvm::Value *float_eval(llvm::IRBuilder<> &builder, llvm::Value *a,
                                  llvm::Value *b) = 0;
};

class AddFunc : public MathFunc {
public:
  virtual llvm::Value *int_eval(llvm::IRBuilder<> &builder, llvm::Value *a,
                                llvm::Value *b) {
    return builder.CreateAdd(a, b);
  }
  virtual llvm::Value *float_eval(llvm::IRBuilder<> &builder, llvm::Value *a,
                                  llvm::Value *b) {
    return builder.CreateFAdd(a, b);
  }
};

class SubtractFunc : public MathFunc {
public:
  virtual llvm::Value *int_eval(llvm::IRBuilder<> &builder, llvm::Value *a,
                                llvm::Value *b) {
    return builder.CreateSub(a, b);
  }
  virtual llvm::Value *float_eval(llvm::IRBuilder<> &builder, llvm::Value *a,
                                  llvm::Value *b) {
    return builder.CreateFSub(a, b);
  }
};

class MultiplyFunc : public MathFunc {
public:
  virtual llvm::Value *int_eval(llvm::IRBuilder<> &builder, llvm::Value *a,
                                llvm::Value *b) {
    return builder.CreateMul(a, b);
  }
  virtual llvm::Value *float_eval(llvm::IRBuilder<> &builder, llvm::Value *a,
                                  llvm::Value *b) {
    return builder.CreateFMul(a, b);
  }
};

class DivideFunc : public Function {
public:
  GenericValue call(Enviroment &env, llvm::IRBuilder<> &builder,
                    std::vector<GenericValue> vals) {
    auto one = to_float(builder, vals.at(0));
    auto two = to_float(builder, vals.at(1));

    auto ret_val = builder.CreateFDiv(one.value, two.value);
    return FloatType().create(ret_val);
  }

  virtual GTPtr return_type(Enviroment &env, std::vector<GTPtr> args) {
    return std::make_shared<FloatType>();
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

    auto first = to_float(builder, one);
    auto second = to_float(builder, two);

    return GenericValue(std::make_shared<BooleanType>(),
                        float_cmp(builder, first.value, second.value));
  }

  virtual llvm::Value *int_cmp(llvm::IRBuilder<> &builder, llvm::Value *a,
                               llvm::Value *b) = 0;
  virtual llvm::Value *float_cmp(llvm::IRBuilder<> &builder, llvm::Value *a,
                                 llvm::Value *b) = 0;
};

class EqualityFunc : public ComparisonFunc {
public:
  virtual llvm::Value *int_cmp(llvm::IRBuilder<> &builder, llvm::Value *a,
                               llvm::Value *b) {
    return builder.CreateICmpEQ(a, b);
  }
  virtual llvm::Value *float_cmp(llvm::IRBuilder<> &builder, llvm::Value *a,
                                 llvm::Value *b) {
    return builder.CreateFCmpOEQ(a, b);
  }
};

class LessThanFunc : public ComparisonFunc {
public:
  virtual llvm::Value *int_cmp(llvm::IRBuilder<> &builder, llvm::Value *a,
                               llvm::Value *b) {
    return builder.CreateICmpSLT(a, b);
  }
  virtual llvm::Value *float_cmp(llvm::IRBuilder<> &builder, llvm::Value *a,
                                 llvm::Value *b) {
    return builder.CreateFCmpOLT(a, b);
  }
};

class GreaterThanFunc : public ComparisonFunc {
public:
  virtual llvm::Value *int_cmp(llvm::IRBuilder<> &builder, llvm::Value *a,
                               llvm::Value *b) {
    return builder.CreateICmpSGT(a, b);
  }
  virtual llvm::Value *float_cmp(llvm::IRBuilder<> &builder, llvm::Value *a,
                                 llvm::Value *b) {
    return builder.CreateFCmpOGT(a, b);
  }
};

class SizeFunc : public Function {
public:
  GenericValue call(Enviroment &env, llvm::IRBuilder<> &builder,
                    std::vector<GenericValue> vals) {
    if (vals.at(0).type->data_type() == DataType::Vector) {
      auto vec = vals.at(0).value;
      auto vec_size = builder.CreateExtractValue(vec, {0});
      return IntegerType().create(vec_size);
    } else if (vals.at(0).type->data_type() == DataType::Tuple) {
      auto tuple_type = static_cast<TupleType *>(vals.at(0).type.get());
      auto tuple_size = tuple_type->sub_types.size();
      return IntegerType().create(tuple_size);
    }
    throw;
  }

  virtual GTPtr return_type(Enviroment &env, std::vector<GTPtr> args) {
    return std::make_shared<IntegerType>();
  }
};

class ReplaceFunc : public Function {
public:
  GenericValue call(Enviroment &env, llvm::IRBuilder<> &builder,
                    std::vector<GenericValue> vals) {

    auto index = vals.at(0);
    auto new_val = vals.at(1);
    auto original = vals.at(2);

    auto new_vec = copy_vec(env, builder, original);
    auto new_vec_arr_ptr = get_vec_arr_ptr(builder, new_vec);
    auto index_ptr = builder.CreateGEP(new_vec_arr_ptr, index.value);
    builder.CreateStore(new_val.value, index_ptr);

    return new_vec;
  }

  virtual GTPtr return_type(Enviroment &env, std::vector<GTPtr> args) {
    return args.at(2);
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
  std::map<std::vector<GTPtr>, llvm::Function *, GTListComparison> fn_map;
  std::map<std::vector<GTPtr>, GTPtr, GTListComparison> return_type_map;
  std::map<std::size_t, ASTProcess> proc_map;
  std::map<std::size_t, std::vector<std::string>> arg_names;
  Enviroment &internal_env;

  LambdaBase(std::map<std::size_t, ASTProcess> proc_map,
             std::map<std::size_t, std::vector<std::string>> arg_names,
             Enviroment &internal_env)
      : proc_map(proc_map), arg_names(arg_names), internal_env(internal_env) {}

  void create_fn(std::vector<GTPtr> types) {
    auto temp_env = internal_env;
    for (int i = 0; i < types.size(); i++) {
      temp_env.value_map[arg_names[types.size()].at(i)] =
          types.at(i)->create(nullptr);
    }
    auto return_type = proc_map[types.size()].return_type(temp_env);
    std::vector<llvm::Type *> llvm_types;
    for (const auto &i : types)
      llvm_types.push_back(i->llvm_type());

    auto fn_ty =
        llvm::FunctionType::get(return_type->llvm_type(), llvm_types, false);
    auto fn = llvm::Function::Create(fn_ty, llvm::Function::ExternalLinkage,
                                     "custom_function", internal_env.module);

    fn_map.insert({types, fn});
    return_type_map.insert({types, return_type});

    auto entry =
        llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", fn);
    auto build = llvm::IRBuilder<>(entry);
    Enviroment new_env = internal_env;

    auto it = fn->arg_begin();
    for (int i = 0; i < types.size(); i++) {
      new_env.value_map[arg_names[types.size()].at(i)] =
          types.at(i)->create(it++);
    }

    build.CreateRet(proc_map[types.size()].to_value(new_env, build).value);
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

    std::map<std::size_t, ASTProcess> proc_map;
    std::map<std::size_t, std::vector<std::string>> arg_names;

    for (auto it = args.begin(); it != args.end();) {
      auto arg_ast = static_cast<ASTVector *>(it->get());
      it++;
      auto proc_ast = static_cast<ASTProcess *>(it->get());
      it++;

      std::vector<std::string> arg_list;
      for (auto i : arg_ast->val) {
        arg_list.push_back(static_cast<ASTSymbol *>(i.get())->val);
      }

      proc_map[arg_list.size()] = *proc_ast;
      arg_names.insert({arg_list.size(), arg_list});
    }

    return make_func(env,
                     std::make_shared<LambdaBase>(proc_map, arg_names, env));
  }

  virtual GTPtr return_type(Enviroment &env,
                            std::vector<std::shared_ptr<ASTNode>> args) {
    return std::make_shared<FunctionType>(
        [](std::vector<GTPtr> a) { return a.at(0); });
  }
};
