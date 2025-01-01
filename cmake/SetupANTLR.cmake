list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/antlr)

add_definitions(-DANTLR4CPP_STATIC)
set(ANTLR4_WITH_STATIC_CRT OFF)

include(ExternalAntlr4Cpp)
find_package(ANTLR REQUIRED)

antlr_target(
    KyotoGrammarLexer 
    ${PROJECT_SOURCE_DIR}/grammar/KyotoLexer.g4 
    LEXER 
    PACKAGE kyoto)

antlr_target(
    KyotoGrammarParser ${PROJECT_SOURCE_DIR}/grammar/KyotoParser.g4 
    PARSER PACKAGE kyoto
    DEPENDS_ANTLR KyotoGrammarLexer
    COMPILE_FLAGS -lib ${ANTLR_KyotoGrammarLexer_OUTPUT_DIR}
    VISITOR)

include_directories(${ANTLR_KyotoGrammarLexer_OUTPUT_DIR})
include_directories(${ANTLR_KyotoGrammarParser_OUTPUT_DIR})

SET( ANTLR_SOURCES
    ${ANTLR_KyotoGrammarLexer_CXX_OUTPUTS}
    ${ANTLR_KyotoGrammarParser_CXX_OUTPUTS})