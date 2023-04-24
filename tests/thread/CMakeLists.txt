AUX_SOURCE_DIRECTORY(./ DIR_SRCS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
add_executable(threadTest ${DIR_SRCS})

target_include_directories(threadTest PRIVATE ./)
target_link_libraries(threadTest PRIVATE Catch2 Threads::Threads THREAD)

catch_discover_tests(threadTest)