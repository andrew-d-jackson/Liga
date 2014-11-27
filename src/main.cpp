#include "LLVM.hpp"

#include "GenericValue.hpp"
#include "GenericType.hpp"
#include "Enviroment.hpp"
#include "Function.hpp"
#include "ASTValues.hpp"
#include "Types.hpp"
#include "StandardFunctions.hpp"
#include "Runtime.hpp"
#include "RefrenceCounter.hpp"
#include "Malloc.hpp"
#include "Parser.hpp"

int main() {
  llvm::InitializeNativeTarget();
  auto &context = llvm::getGlobalContext();
  auto module = std::make_unique<llvm::Module>("test", context);
  auto malloc_fn = MallocFunc(module.get());
  Scope global_scope;

  llvm::ExecutionEngine *EE = llvm::EngineBuilder(module.get()).create();

  llvm::FunctionPassManager OurFPM(module.get());
  OurFPM.add(llvm::createBasicAliasAnalysisPass());
  OurFPM.add(llvm::createAAEvalPass());
  // OurFPM.add(llvm::createInstCountPass()());
  // OurFPM.add(llvm::createReassociatePass());
  // OurFPM.add(llvm::createGVNPass());
  // OurFPM.add(llvm::createCFGSimplificationPass());
  // OurFPM.doInitialization();

  Enviroment env{module.get(), malloc_fn, global_scope};
  // Set the global so the code gen can use this.
  //  TheFPM = &OurFPM;

  auto print = std::make_shared<PrintFunc>();
  print->create_mapping(env, std::make_shared<IntegerType>());
  print->create_mapping(env, std::make_shared<BooleanType>());
  print->create_mapping(env, std::make_shared<FloatType>());
  print->create_mapping(env, std::make_shared<CharType>());
  print->create_vector_mapping(
      env, std::make_shared<VectorType>(std::make_shared<CharType>()));

  env.value_map["print"] = make_func(env, print);
  env.value_map["at"] = make_func(env, std::make_shared<AtFunc>());
  env.value_map["+"] = make_func(env, std::make_shared<AddFunc>());
  env.value_map["="] = make_macro(env, std::make_shared<DefineMacro>());
  env.value_map["fn"] = make_macro(env, std::make_shared<LambdaMacro>());
  env.value_map["if"] = make_macro(env, std::make_shared<IfMacro>());
  env.value_map["append"] = make_func(env, std::make_shared<AppendFunc>());
  env.value_map[">"] = make_func(env, std::make_shared<GreaterThanFunc>());
  env.value_map["=="] = make_func(env, std::make_shared<EqualityFunc>());
  env.value_map["<"] = make_func(env, std::make_shared<LessThanFunc>());

  auto main_ty = llvm::FunctionType::get(
      llvm::IntegerType::getInt1Ty(llvm::getGlobalContext()), false);
  auto main_fn = llvm::Function::Create(
      main_ty, llvm::Function::ExternalLinkage, "main", module.get());

  auto main_entry = llvm::BasicBlock::Create(context, "entry", main_fn);
  auto builder = llvm::IRBuilder<>(main_entry);

  auto test_parse =
      parse("[print [if [>>= 5.9 5.8999] \"fuck off bro\" \"ya rly\"]]");

  std::cout << "Parsed Program: " << std::endl << std::endl;
  for (const auto &i : test_parse) {
    std::cout << i->as_string() << std::endl;
  }
  std::cout << "----------- " << std::endl;

  for (const auto &i : test_parse) {
    i->to_value(env, builder);
  }

  /// builder.CreateRet(builder.getTrue());
  std::cout << global_scope.values.size();
  global_scope.create_return(
      env, builder,
      GenericValue{std::make_shared<BooleanType>(), builder.getTrue()});

  // llvm::verifyFunction(*main_fn);
  // OurFPM.run(*main_fn);
  std::cout << "LLVM IR:" << std::endl << std::endl;
  std::cout << string_representation(*module);
  std::cout << "----------- " << std::endl;

  EE->addGlobalMapping(print->fn_map[std::make_shared<IntegerType>()],
                       (void *)&print_i32);
  EE->addGlobalMapping(print->fn_map[std::make_shared<BooleanType>()],
                       (void *)&print_bool);
  EE->addGlobalMapping(print->fn_map[std::make_shared<FloatType>()],
                       (void *)&print_float);
  EE->addGlobalMapping(print->fn_map[std::make_shared<CharType>()],
                       (void *)&print_char);
  EE->addGlobalMapping(env.malloc_fn.fn, (void *)&print_and_malloc);
  EE->addGlobalMapping(env.malloc_fn.free_fn, (void *)&print_and_free);

  std::cout << "Standard Output of Program:" << std::endl << std::endl;
  auto gv = EE->runFunction(main_fn, {});
  // int sd = (int)gv.IntVal.getZExtValue();
  // std::cout << "Result: " << sd << "\n";

  return 0;
}
