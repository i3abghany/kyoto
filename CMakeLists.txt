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
    src/AST/StringLiteralNode.cpp
    src/AST/DeclarationNodes.cpp
    src/AST/ClassDefinitionNode.cpp
    src/AST/ExpressionNode.cpp
    src/AST/FunctionCall.cpp
    src/AST/IfStatementNode.cpp
    src/AST/ForStatementNode.cpp
    src/AST/ASTBinaryArithNode.cpp
    src/AST/ASTBinaryCmpNode.cpp
    src/AST/ASTBinaryLogicalNode.cpp
    src/AST/NumberNode.cpp
    src/Resolution/ConstructorResolution.cpp
    src/Resolution/ClassIdentifierVisitor.cpp
    src/ModuleCompiler.cpp
    src/SymbolTable.cpp
    src/KType.cpp
    src/TypeResolver.cpp
    src/utils/File.cpp
    src/Analysis/FunctionTermination.cpp
    ${ANTLR_SOURCES})

add_executable(
    kyoto
    main.cpp
    ${SOURCES})

target_link_libraries(kyoto antlr4_static ${llvm_libs} ${Boost_LIBRARIES} fmt::fmt-header-only)

set(TEST_SOURCES
    test/TestDriver.cpp
    test/TestReturn.cpp
    test/TestFunctionTermination.cpp
    test/TestAssignment.cpp
    test/TestIfStatement.cpp
    test/TestLogical.cpp
    test/TestScope.cpp
    test/TestFor.cpp
    test/TestFunction.cpp
    test/TestPointers.cpp
)

add_executable(
    ktest
    ${SOURCES}
    ${TEST_SOURCES}
)

target_link_libraries(ktest PRIVATE gtest_main gtest antlr4_static ${llvm_libs} ${Boost_LIBRARIES} fmt::fmt-header-only)

include(GoogleTest)
gtest_discover_tests(ktest)
