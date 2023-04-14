AUX_SOURCE_DIRECTORY(./ DIR_SRCS)

add_executable(threadTest ${DIR_SRCS})


include(catch2)

target_include_directories(threadTest PRIVATE ./)
target_link_libraries(threadTest PRIVATE Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
include(Catch)
catch_discover_tests(threadTest)