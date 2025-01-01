CMAKE_MINIMUM_REQUIRED(VERSION 3.20 FATAL_ERROR)
project(Kyoto)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/antlr)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/SetupLLVM.cmake)
include(cmake/SetupANTLR.cmake)
include(cmake/SetupGTest.cmake)
include(cmake/SetupBoost.cmake)
find_package(fmt CONFIG REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${ANTLR4_INCLUDE_DIRS})

set(SOURCES
    src/Visitor.cpp
    src/AST/ASTNode.cpp
    src/AST/ControlFlowNodes.cpp
    src/AST/ASTBinaryArithNode.cpp
    src/AST/ASTBinaryCmpNode.cpp
    src/AST/NumberNode.cpp
    src/ModuleCompiler.cpp
    src/SymbolTable.cpp
    src/KType.cpp
    src/TypeResolver.cpp
    src/utils/File.cpp
    ${ANTLR_SOURCES})

add_executable(
    kyoto
    main.cpp
    ${SOURCES})

target_link_libraries(kyoto antlr4_static ${llvm_libs} ${Boost_LIBRARIES} fmt::fmt-header-only)

add_executable(
    ktest
    ${SOURCES}
    test/TestReturn.cpp
)

target_link_libraries(ktest PRIVATE gtest_main gtest antlr4_static ${llvm_libs} ${Boost_LIBRARIES} fmt::fmt-header-only)