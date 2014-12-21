#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "ASTNode.hpp"

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <functional>
#include <exception>

class Function {
public:
  virtual GenericValue call(Enviroment &env, llvm::IRBuilder<> &,
                            std::vector<GenericValue>) = 0;
  virtual GTPtr return_type(Enviroment &env, std::vector<GTPtr>) = 0;
  virtual bool is_pure(Enviroment &env, std::vector<GTPtr>) { return false; }
};

class Macro {
public:
  virtual GenericValue call(Enviroment &, llvm::IRBuilder<> &,
                            std::vector<std::shared_ptr<ASTNode>>) = 0;
  virtual GTPtr return_type(Enviroment &env,
                            std::vector<std::shared_ptr<ASTNode>>) = 0;
  virtual bool is_pure(Enviroment &env, std::vector<std::shared_ptr<ASTNode>>) {
    return false;
  }
};

class MacroType : public GenericType {
private:
  std::function<GTPtr(std::vector<std::shared_ptr<ASTNode>>)> return_type_fn;
  std::function<bool(std::vector<std::shared_ptr<ASTNode>>)> purity_fn;

public:
  MacroType(
      std::function<GTPtr(std::vector<std::shared_ptr<ASTNode>>)>
          return_type_fn,
      std::function<bool(std::vector<std::shared_ptr<ASTNode>>)> purity_fn)
      : return_type_fn(return_type_fn), purity_fn(purity_fn) {}
  virtual DataType data_type() const { return DataType::Macro; }
  virtual bool operator==(const GenericType &other) const {
    return other.data_type() == data_type();
  }
  virtual llvm::Type *llvm_type() const { throw "Macro has no llvm Type"; }
  virtual GenericValue create(llvm::Value *val) const {
    throw "Cannot Create Macro from value";
  }
  GTPtr return_type(Enviroment &env,
                    std::vector<std::shared_ptr<ASTNode>> args) const {
    return return_type_fn(args);
  }
  bool is_pure(Enviroment &env,
               std::vector<std::shared_ptr<ASTNode>> args) const {
    return purity_fn(args);
  }
};

class FunctionType : public GenericType {
private:
  std::function<GTPtr(std::vector<GTPtr>)> return_type_fn;
  std::function<bool(std::vector<GTPtr>)> purity_fn;

public:
  FunctionType(std::function<GTPtr(std::vector<GTPtr>)> return_type_fn,
               std::function<bool(std::vector<GTPtr>)> purity_fn)
      : return_type_fn(return_type_fn), purity_fn(purity_fn) {}
  virtual DataType data_type() const { return DataType::Function; }
  virtual bool operator==(const GenericType &other) const {
    return other.data_type() == data_type();
  }
  virtual llvm::Type *llvm_type() const { throw "Function has no llvm Type"; }
  virtual GenericValue create(llvm::Value *val) const {
    throw "Cannot Create Function from value";
  }

  GTPtr return_type(Enviroment &env, std::vector<GTPtr> args) const {
    return return_type_fn(args);
  }

  bool is_pure(Enviroment &env, std::vector<GTPtr> args) const {
    return purity_fn(args);
  }
};

extern GenericValue make_func(Enviroment &env, std::shared_ptr<Function> f);

extern GenericValue make_macro(Enviroment &env, std::shared_ptr<Macro> f);