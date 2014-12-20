#pragma once
#include "LLVM.hpp"

#include <functional>

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"

class VectorType : public GenericType {
public:
  VectorType(GTPtr sub_type) : sub_type(sub_type) {}
  virtual DataType data_type() const;
  virtual bool operator==(const GenericType &other) const;
  virtual bool operator<(const GenericType &other) const;
  virtual llvm::Type *llvm_type() const;
  virtual GenericValue create(llvm::Value *val) const;
  virtual GenericValue copy(Enviroment &env, llvm::IRBuilder<> &builder,
                            GenericValue val) const;
  void for_each_element(Enviroment &env, llvm::IRBuilder<> &builder,
                        GenericValue vec,
                        std::function<void(GenericValue)> fn) const;
  llvm::Function *destructor(Enviroment &env, llvm::IRBuilder<> &builder,
                             GenericValue val) const;
  virtual void destroy(Enviroment &env, llvm::IRBuilder<> &builder,
                       GenericValue val) const;

public:
  GTPtr sub_type;
};

extern void for_each_vec(Enviroment &env, llvm::IRBuilder<> &builder,
                         GenericValue vec,
                         std::function<void(GenericValue, GenericValue)> fn);

extern GenericValue create_vec(Enviroment &env, llvm::IRBuilder<> &builder,
                               GTPtr vec_type, llvm::Value *size,
                               llvm::Value *arr);

extern llvm::Value *get_vec_arr_ptr(llvm::IRBuilder<> &builder,
                                    GenericValue original);

extern llvm::Value *get_vec_size(llvm::IRBuilder<> &builder,
                                 GenericValue original);

extern GTPtr get_vec_sub_type(llvm::IRBuilder<> &builder,
                              GenericValue original);

extern GenericValue copy_vec(Enviroment &env, llvm::IRBuilder<> &builder,
                             GenericValue original);