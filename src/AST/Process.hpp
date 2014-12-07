#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "ASTNode.hpp"

class ASTProcess : public ASTNode {
public:
  std::vector<std::shared_ptr<ASTNode>> val;

public:
  ASTProcess() : val() {}
  ASTProcess(const ASTProcess &other) : val(other.val) {}
  ASTProcess(std::vector<std::shared_ptr<ASTNode>> val) : val(val) {}

  virtual DataType type() const { return DataType::Process; }

  bool is_prefix_call(Enviroment &env, DataType ty) const {
    auto first_dt = val.at(0)->return_type(env)->data_type();
    if (first_dt == ty) {
      if (val.size() > 1)
        return true;
    }
    return false;
  }

  bool is_infix_func_call(Enviroment &env) const {
    if (val.size() != 3)
      return false;
    if (val.at(0)->return_type(env)->data_type() == DataType::Function)
      return false;
    if (val.at(1)->return_type(env)->data_type() == DataType::Function)
      return true;
    return false;
  }

  std::vector<GTPtr> get_value_types(Enviroment &env) const {
    std::vector<GTPtr> val_types;
    for (const auto &i : val) {
      val_types.push_back(i->return_type(env));
    }
    return val_types;
  }

  std::vector<GenericValue> generate_values(Enviroment &env,
                                            llvm::IRBuilder<> &builder) const {
    std::vector<GenericValue> values;
    for (const auto &i : val) {
      values.push_back(i->to_value(env, builder));
    }
    return values;
  }

  GenericValue to_value(Enviroment &env, llvm::IRBuilder<> &builder) {
    if (is_prefix_call(env, DataType::Macro)) {
      auto mac_args =
          std::vector<std::shared_ptr<ASTNode>>(val.begin() + 1, val.end());
      return val.at(0)
          ->to_value(env, builder)
          .macro->call(env, builder, mac_args);
    }

    auto values = generate_values(env, builder);

    if (is_prefix_call(env, DataType::Function)) {
      auto fn_args =
          std::vector<GenericValue>(values.begin() + 1, values.end());
      return values.at(0).func->call(env, builder, fn_args);
    }

    if (is_infix_func_call(env)) {
      std::vector<GenericValue> fn_args;
      fn_args.push_back(values.at(0));
      fn_args.push_back(values.at(2));
      return values.at(1).func->call(env, builder, fn_args);
    }

    return *(values.end() - 1);
  };

  virtual GTPtr return_type(Enviroment &env) const {

    if (is_prefix_call(env, DataType::Macro)) {
      auto args_list =
          std::vector<std::shared_ptr<ASTNode>>(val.begin() + 1, val.end());
      return static_cast<MacroType *>(val.at(0)->return_type(env).get())
          ->return_type(env, args_list);
    }

    auto val_types = get_value_types(env);

    if (is_prefix_call(env, DataType::Function)) {
      auto args_list =
          std::vector<GTPtr>(val_types.begin() + 1, val_types.end());
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

  virtual std::string as_string() const {
    std::string ret = "[ ";
    for (const auto &i : val) {
      ret += i->as_string();
      ret += ' ';
    }
    ret += "]";
    return ret;
  }
};
