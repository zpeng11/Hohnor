# Create static library .o library for upper level to use
AUX_SOURCE_DIRECTORY(./ DIR_SRCS)
add_library(THREAD STATIC ${DIR_SRCS})

#link to pthread library
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(THREAD PUBLIC Threads::Threads) # Public link pthread to avoid case that use dlopen to open thread.so, thread.so is not linked to pthread

target_include_directories(THREAD PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
