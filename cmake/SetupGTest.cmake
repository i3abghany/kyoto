if(TARGET gtest AND TARGET gtest_main)
    return()
endif()

if(NOT GTEST_FOUND)
    message(STATUS "Downloading and setting up Google Test")
    
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    FetchContent_MakeAvailable(googletest)
endif()

enable_testing()
add_compile_options(-std=c++23)