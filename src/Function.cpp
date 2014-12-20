#include "Function.hpp"

GenericValue make_func(Enviroment &env, std::shared_ptr<Function> f) {
  return GenericValue(
      std::make_shared<FunctionType>([f, &env](std::vector<GTPtr> a) {
        return f->return_type(env, a);
      }),
      f);
};

GenericValue make_macro(Enviroment &env, std::shared_ptr<Macro> f) {
  return GenericValue(std::make_shared<MacroType>([f, &env](
                          std::vector<std::shared_ptr<ASTNode>> a) {
                        return f->return_type(env, a);
                      }),
                      f);
};