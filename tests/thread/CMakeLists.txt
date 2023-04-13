AUX_SOURCE_DIRECTORY(./ DIR_SRCS)

add_executable(threadTest ${DIR_SRCS})


Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.13.9 # or a later release
)

FetchContent_MakeAvailable(Catch2)

target_link_libraries(threadTest PRIVATE Catch2WithMain)

# include(Catch)
# catch_discover_tests(threadTest)