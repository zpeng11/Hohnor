include_directories(${CMAKE_CURRENT_SOURCE_DIR}/hohnor)
add_subdirectory(hohnor/thread)
# add_subdirectory(hohnor/time)
# add_subdirectory(hohnor/log)
# add_subdirectory(hohnor/file)
# add_subdirectory(hohnor/process)
# add_subdirectory(hohnor/net)
# add_subdirectory(hohnor/io)
# add_subdirectory(hohnor/core)
include_directories(hohnor/common)


set(
    hohnor_modules
    THREAD
)

foreach(module IN LISTS hohnor_modules)
    string(TOLOWER ${module} module_lower)
    target_include_directories(${module} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    add_library("Hohnor::${module_lower}" ALIAS ${module})
endforeach()

add_library(Hohnor hohnor/hohnor.hpp)
target_link_libraries(Hohnor ${hohnor_modules})