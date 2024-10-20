# Create static library .o library for upper level to use
AUX_SOURCE_DIRECTORY(./ DIR_SRCS)
add_library(PROCESS STATIC ${DIR_SRCS})

# Since using Current thread link to THREAD
target_link_libraries(PROCESS PUBLIC THREAD)
target_link_libraries(PROCESS PUBLIC FILE)
target_link_libraries(PROCESS PUBLIC TIME)

#add current directory *.h for upper level to include
target_include_directories(PROCESS PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})