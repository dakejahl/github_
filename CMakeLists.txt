cmake_minimum_required(VERSION 3.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

set(DEPS_BUILD_PATH "${PROJECT_BINARY_DIR}/third_party" CACHE PATH "Install path for the dependencies. Ignored if SUPERBUILD=OFF.")
set(DEPS_INSTALL_PATH "${DEPS_BUILD_PATH}/install" CACHE PATH "Install path for the dependencies. Ignored if SUPERBUILD=OFF.")

add_subdirectory(third_party)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.58.0 COMPONENTS filesystem)
find_package(CURL)
find_package(JsonCpp REQUIRED)

add_definitions("-DDEBUG")

if(Boost_FOUND AND CURL_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    include_directories(${CURL_INCLUDE_DIRS})

	add_executable(curl_example
		curl_example.cpp
	)
    target_link_libraries(curl_example ${Boost_LIBRARIES} ${CURL_LIBRARIES} JsonCpp::jsoncpp)
endif()
