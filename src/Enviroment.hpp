#pragma once
#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"

class MallocFunc;
class GenericValue;

class Scope {
public:
  std::vector<GenericValue> values;

  void add(GenericValue gv) { values.push_back(gv); }

  void add(std::vector<GenericValue> gvs) {
    for (auto gv : gvs)
      values.push_back(gv);
  }

  void create_return(Enviroment &env, llvm::IRBuilder<> &builder,
                     GenericValue gv) {
    gv.type->copy(env, builder, gv);
    for (auto scoped_value : values) {
      scoped_value.type->destroy(env, builder, scoped_value);
    }
    builder.CreateRet(gv.value);
  }

  void destroy_all_but(Enviroment &env, llvm::IRBuilder<> &builder,
                       GenericValue gv) {
    gv.type->copy(env, builder, gv);
    for (auto i : values) {
      i.type->destroy(env, builder, i);
    }
  }
};

class Enviroment {
public:
  llvm::Module *module;
  MallocFunc &malloc_fn;
  Scope &scope;
  std::map<std::string, GenericValue> value_map;

  Enviroment with_new_scope(Scope &scope) {
    Enviroment ret{module, malloc_fn, scope, value_map};
    return ret;
  }
};
