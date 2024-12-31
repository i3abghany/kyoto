CMAKE_MINIMUM_REQUIRED(VERSION 3.20 FATAL_ERROR)
project(Kyoto)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/antlr)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/SetupLLVM.cmake)
include(cmake/SetupANTLR.cmake)
find_package(fmt CONFIG REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${ANTLR4_INCLUDE_DIRS})
include_directories(${ANTLR_KyotoGrammarLexer_OUTPUT_DIR})
include_directories(${ANTLR_KyotoGrammarParser_OUTPUT_DIR})

add_executable(
    kyoto
    main.cpp
    src/Visitor.cpp
    src/AST/ASTNode.cpp
    src/AST/ASTBinaryArithNode.cpp
    src/AST/ASTBinaryCmpNode.cpp
    src/AST/NumberNode.cpp
    src/ModuleCompiler.cpp
    src/SymbolTable.cpp
    src/KType.cpp
    src/TypeResolver.cpp
    ${ANTLR_KyotoGrammarLexer_CXX_OUTPUTS}
    ${ANTLR_KyotoGrammarParser_CXX_OUTPUTS})

target_link_libraries(kyoto antlr4_static ${llvm_libs} fmt::fmt-header-only)
