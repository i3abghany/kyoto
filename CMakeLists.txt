CMAKE_MINIMUM_REQUIRED(VERSION 3.20 FATAL_ERROR)
project(Kyoto)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_definitions(-DANTLR4CPP_STATIC)
set(ANTLR4_WITH_STATIC_CRT OFF)

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
llvm_map_components_to_libnames(llvm_libs support core irreader linker)

find_package(fmt CONFIG REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include)

include(ExternalAntlr4Cpp)
include_directories(${ANTLR4_INCLUDE_DIRS})

find_package(ANTLR REQUIRED)

antlr_target(KyotoGrammarLexer ${PROJECT_SOURCE_DIR}/grammar/KyotoLexer.g4 LEXER
             PACKAGE kyoto
             )
antlr_target(KyotoGrammarParser ${PROJECT_SOURCE_DIR}/grammar/KyotoParser.g4 PARSER
             PACKAGE kyoto
             DEPENDS_ANTLR KyotoGrammarLexer
             COMPILE_FLAGS -lib ${ANTLR_KyotoGrammarLexer_OUTPUT_DIR}
             VISITOR)

# include generated files in project environment
include_directories(${ANTLR_KyotoGrammarLexer_OUTPUT_DIR})
include_directories(${ANTLR_KyotoGrammarParser_OUTPUT_DIR})

# add generated grammar to demo binary target
add_executable(kyoto main.cpp
               src/Visitor.cpp
               src/AST/ASTNode.cpp
               src/AST/ASTBinaryArithNode.cpp
               src/ModuleCompiler.cpp
               src/SymbolTable.cpp
               src/Type.cpp
               ${ANTLR_KyotoGrammarLexer_CXX_OUTPUTS}
               ${ANTLR_KyotoGrammarParser_CXX_OUTPUTS})

target_link_libraries(kyoto antlr4_static ${llvm_libs} fmt::fmt-header-only)
