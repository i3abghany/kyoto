CMAKE_MINIMUM_REQUIRED(VERSION 3.20 FATAL_ERROR)
project(Kyoto)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/antlr)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fno-threadsafe-statics -fno-strict-aliasing -fno-math-errno -ffunction-sections -fdata-sections")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/SetupLLVM.cmake)
include(cmake/SetupANTLR.cmake)
include(cmake/SetupGTest.cmake)
include(cmake/SetupBoost.cmake)
find_package(fmt CONFIG REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${ANTLR4_INCLUDE_DIRS})

set(SOURCES
    src/AST/ASTNode.cpp
    src/AST/ClassDefinitionNode.cpp
    src/AST/DeclarationNodes.cpp
    src/AST/Expressions/ArrayNode.cpp
    src/AST/Expressions/AssignmentNode.cpp
    src/AST/Expressions/BinaryArithNode.cpp
    src/AST/Expressions/BinaryCmpNode.cpp
    src/AST/Expressions/BinaryLogicalNode.cpp
    src/AST/Expressions/ExpressionNode.cpp
    src/AST/Expressions/FunctionCallNode.cpp
    src/AST/Expressions/IdentifierNode.cpp
    src/AST/Expressions/MatchNode.cpp
    src/AST/Expressions/MemberAccessNode.cpp
    src/AST/Expressions/MethodCallNode.cpp
    src/AST/Expressions/NewNode.cpp
    src/AST/Expressions/NewArrayNode.cpp
    src/AST/Expressions/NumberNode.cpp
    src/AST/Expressions/StringLiteralNode.cpp
    src/AST/Expressions/UnaryNode.cpp
    src/AST/ForStatementNode.cpp
    src/AST/FreeStatementNode.cpp
    src/AST/IfStatementNode.cpp
    src/AST/ReturnStatement.cpp
    src/Analysis/FunctionTermination.cpp
    src/KType.cpp
    src/ModuleCompiler.cpp
    src/SymbolTable.cpp
    src/TypeResolver.cpp
    src/Visitor.cpp
    src/utils/File.cpp
    ${ANTLR_SOURCES})

add_executable(
    cyoto
    main.cpp
    ${SOURCES})

target_link_libraries(cyoto antlr4_static ${llvm_libs} ${Boost_LIBRARIES} fmt::fmt-header-only)

set(TEST_SOURCES
    test/TestDriver.cpp
    test/TestReturn.cpp
    test/TestFunctionTermination.cpp
    test/TestAssignment.cpp
    test/TestIfStatement.cpp
    test/TestLogical.cpp
    test/TestScope.cpp
    test/TestFor.cpp
    test/TestNew.cpp
    test/TestClass.cpp
    test/TestFunction.cpp
    test/TestPointers.cpp
    test/TestMatch.cpp
)

add_executable(
    ktest
    ${SOURCES}
    ${TEST_SOURCES}
)

target_link_libraries(ktest PRIVATE gtest_main gtest antlr4_static ${llvm_libs} ${Boost_LIBRARIES} fmt::fmt-header-only)

include(GoogleTest)
gtest_discover_tests(ktest)

