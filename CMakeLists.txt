CMAKE_MINIMUM_REQUIRED(VERSION 3.20 FATAL_ERROR)
project(Kyoto)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/antlr)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_definitions(-DANTLR4CPP_STATIC)
set(ANTLR4_WITH_STATIC_CRT OFF)

include(cmake/SetupLLVM.cmake)
find_package(fmt CONFIG REQUIRED)
include(ExternalAntlr4Cpp)
find_package(ANTLR REQUIRED)

antlr_target(KyotoGrammarLexer ${PROJECT_SOURCE_DIR}/grammar/KyotoLexer.g4 LEXER
             PACKAGE kyoto
             )
antlr_target(KyotoGrammarParser ${PROJECT_SOURCE_DIR}/grammar/KyotoParser.g4 PARSER
             PACKAGE kyoto
             DEPENDS_ANTLR KyotoGrammarLexer
             COMPILE_FLAGS -lib ${ANTLR_KyotoGrammarLexer_OUTPUT_DIR}
             VISITOR)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${ANTLR4_INCLUDE_DIRS})
include_directories(${ANTLR_KyotoGrammarLexer_OUTPUT_DIR})
include_directories(${ANTLR_KyotoGrammarParser_OUTPUT_DIR})

add_executable(kyoto main.cpp
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
