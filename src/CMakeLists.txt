cmake_minimum_required(VERSION 2.8.4)
project(untitled)


find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")


include_directories(${LLVM_INCLUDE_DIRS})

add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs support core irreader jit interpreter native)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
include_directories(${CMAKE_SOURCE_DIR})

set(SOURCE_FILES main.cpp main.liga ASTNode.hpp ASTValues.hpp Enviroment.hpp Function.hpp GenericType.hpp GenericValue.hpp LLVM.hpp LLVMUtils.hpp Malloc.hpp Parser.hpp RefrenceCounter.hpp Runtime.hpp StandardFunctions.hpp Types.hpp AST/Boolean.hpp AST/Integer.hpp AST/Float.hpp AST/Process.hpp AST/Char.hpp AST/Symbol.hpp AST/Vector.hpp Types/Char.hpp Types/Boolean.hpp Types/Integer.hpp Types/Float.hpp Types/Vector.hpp)
add_executable(untitled ${SOURCE_FILES})

add_custom_command(TARGET untitled
                   PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/main.liga ${PROJECT_BINARY_DIR})


target_link_libraries(untitled ${llvm_libs})
