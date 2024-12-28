CMAKE_MINIMUM_REQUIRED(VERSION 3.20 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 23)
add_definitions(-DANTLR4CPP_STATIC)
set(ANTLR4_WITH_STATIC_CRT OFF)

include(ExternalAntlr4Cpp)
include_directories(${ANTLR4_INCLUDE_DIRS})

find_package(ANTLR REQUIRED)

antlr_target(KyotoGrammarLexer ${PROJECT_SOURCE_DIR}/grammar/KyotoLexer.g4 LEXER
             PACKAGE antlrcpptest)
antlr_target(KyotoGrammarParser ${PROJECT_SOURCE_DIR}/grammar/KyotoParser.g4 PARSER
             PACKAGE antlrcpptest
             DEPENDS_ANTLR KyotoGrammarLexer
             COMPILE_FLAGS -lib ${ANTLR_KyotoGrammarLexer_OUTPUT_DIR})

# include generated files in project environment
include_directories(${ANTLR_KyotoGrammarLexer_OUTPUT_DIR})
include_directories(${ANTLR_KyotoGrammarParser_OUTPUT_DIR})

# add generated grammar to demo binary target
add_executable(demo main.cpp
               ${ANTLR_KyotoGrammarLexer_CXX_OUTPUTS}
               ${ANTLR_KyotoGrammarParser_CXX_OUTPUTS})
target_link_libraries(demo antlr4_static)
