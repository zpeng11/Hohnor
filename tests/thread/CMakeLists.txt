AUX_SOURCE_DIRECTORY(./ DIR_SRCS)

add_executable(threadTest ${DIR_SRCS})

target_include_directories(threadTest PRIVATE ./)
target_link_libraries(threadTest PRIVATE Catch2)

catch_discover_tests(threadTest)