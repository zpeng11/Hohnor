# Create static library .o library for upper level to use
AUX_SOURCE_DIRECTORY(./ DIR_SRCS)
add_library(THREAD STATIC ${DIR_SRCS})

#link to pthread library
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(THREAD PUBLIC Threads::Threads)

#add current directory *.h for upper level to include
target_include_directories(THREAD INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

#add include to common src/*.h files
target_include_directories(THREAD PUBLIC "${PROJECT_SOURCE_DIR}/src")
