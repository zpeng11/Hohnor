# Create static library .o library for upper level to use
AUX_SOURCE_DIRECTORY(./ DIR_SRCS)
add_library(SOCKET STATIC ${DIR_SRCS})

# Since using Current thread link to THREAD
target_link_libraries(SOCKET PUBLIC THREAD)
target_link_libraries(SOCKET PUBLIC FILE)
target_link_libraries(SOCKET PUBLIC TIME)
target_link_libraries(SOCKET PUBLIC LOG)

#add current directory *.h for upper level to include
target_include_directories(SOCKET INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})