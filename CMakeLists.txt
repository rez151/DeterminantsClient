cmake_minimum_required(VERSION 2.8.4)
project(DeterminantsClient)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)


message(STATUS "start running cmake...")

find_package(Boost 1.58.0 COMPONENTS system filesystem REQUIRED)

if(Boost_FOUND)

    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")

    include_directories(${Boost_INCLUDE_DIRS})

endif()

add_executable(DeterminantsClient ${SOURCE_FILES})

if(Boost_FOUND)
    target_link_libraries(DeterminantsClient rt)
    target_link_libraries(DeterminantsClient pthread)

    target_link_libraries(DeterminantsClient ${Boost_LIBRARIES})

endif()

