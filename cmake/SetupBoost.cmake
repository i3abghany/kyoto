set(REQUIRED_BOOST_COMPONENTS system filesystem)
find_package(Boost 1.74.0 REQUIRED COMPONENTS ${REQUIRED_BOOST_COMPONENTS})

if (Boost_FOUND)
    message(STATUS "Boost found. Include dir: ${Boost_INCLUDE_DIRS}")
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})

    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME OFF)

    message(STATUS "Boost libraries found: ${Boost_LIBRARIES}")
else()
    message(FATAL_ERROR "Boost not found")
endif()

