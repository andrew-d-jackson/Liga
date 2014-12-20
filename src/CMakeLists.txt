cmake_minimum_required(VERSION 2.8.4)
project(Liga)


find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")


include_directories(${LLVM_INCLUDE_DIRS})

add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs support core irreader jit interpreter native)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
include_directories(${CMAKE_SOURCE_DIR})

set(SOURCE_FILES main.cpp main.liga ASTNode.hpp ASTValues.hpp Enviroment.hpp Function.hpp Function.cpp GenericType.hpp GenericValue.hpp LLVM.hpp LLVMUtils.hpp LLVMUtils.cpp Malloc.hpp Parser.hpp RefrenceCounter.hpp RefrenceCounter.cpp Runtime.hpp StandardFunctions.hpp Types.hpp AST/Boolean.hpp AST/Boolean.cpp AST/Integer.hpp AST/Integer.cpp AST/Float.hpp AST/Float.cpp AST/Process.hpp AST/Process.cpp AST/Char.hpp AST/Char.cpp AST/Symbol.hpp AST/Symbol.cpp AST/Vector.hpp AST/Vector.cpp AST/Tuple.hpp AST/Tuple.cpp Types/Char.hpp Types/Char.cpp Types/Boolean.hpp Types/Boolean.cpp Types/Integer.cpp Types/Integer.hpp Types/Float.hpp Types/Float.cpp Types/Void.hpp Types/Void.cpp Types/Vector.cpp Types/Vector.hpp Types/Tuple.hpp Types/Tuple.cpp)
add_executable(Liga ${SOURCE_FILES})

add_custom_command(TARGET Liga
                   PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/main.liga ${PROJECT_BINARY_DIR})


target_link_libraries(Liga ${llvm_libs})
