set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.58.0 COMPONENTS filesystem)
find_package(CURL)

add_definitions("-DDEBUG")

if(Boost_FOUND AND CURL_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    include_directories(${CURL_INCLUDE_DIRS})

	add_executable(git_updater
		GitUpdater.cpp
	)
    target_link_libraries(git_updater ${Boost_LIBRARIES} ${CURL_LIBRARIES})
endif()
