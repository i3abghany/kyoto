set(KYOTO_REQUIRED_LLVM_VERSION 20)
set(KYOTO_MAX_LLVM_VERSION 21)

# Prefer the distro-packaged LLVM 20 install so `apt install llvm-20-dev`
# works without manual `-DLLVM_DIR=...` configuration.
find_program(KYOTO_LLVM_CONFIG NAMES llvm-config-20 llvm-config)

if(KYOTO_LLVM_CONFIG)
    execute_process(
        COMMAND ${KYOTO_LLVM_CONFIG} --version
        OUTPUT_VARIABLE KYOTO_LLVM_CONFIG_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(KYOTO_LLVM_CONFIG_VERSION VERSION_GREATER_EQUAL ${KYOTO_REQUIRED_LLVM_VERSION}
       AND KYOTO_LLVM_CONFIG_VERSION VERSION_LESS ${KYOTO_MAX_LLVM_VERSION})
        execute_process(
            COMMAND ${KYOTO_LLVM_CONFIG} --cmakedir
            OUTPUT_VARIABLE KYOTO_LLVM_CMAKE_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if(EXISTS "${KYOTO_LLVM_CMAKE_DIR}/LLVMConfig.cmake")
            set(LLVM_DIR "${KYOTO_LLVM_CMAKE_DIR}" CACHE PATH "Path to LLVM CMake config" FORCE)
        endif()
    endif()
endif()

find_package(
    LLVM REQUIRED CONFIG
    HINTS
        /usr/lib/llvm-20/lib/cmake/llvm
        /usr/lib/llvm-20/cmake
)

if(NOT LLVM_PACKAGE_VERSION VERSION_GREATER_EQUAL ${KYOTO_REQUIRED_LLVM_VERSION}
   OR NOT LLVM_PACKAGE_VERSION VERSION_LESS ${KYOTO_MAX_LLVM_VERSION})
    message(FATAL_ERROR
        "Kyoto requires LLVM 20.x, but found LLVM ${LLVM_PACKAGE_VERSION}."
    )
endif()

if(NOT EXISTS "${LLVM_INCLUDE_DIRS}/llvm/IR/Analysis.h")
    message(FATAL_ERROR
        "LLVM headers were not found under ${LLVM_INCLUDE_DIRS}. "
        "Install llvm-20-dev or set LLVM_DIR to an LLVM 20 CMake package directory."
    )
endif()

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
llvm_map_components_to_libnames(llvm_libs support core irreader passes linker)
