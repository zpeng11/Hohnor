# Create static library .o library for upper level to use
AUX_SOURCE_DIRECTORY(./ DIR_SRCS)
add_library(TIME STATIC ${DIR_SRCS})

# Since using Current thread link to THREAD
#target_link_libraries(TIME PUBLIC THREAD)

#add current directory *.h for upper level to include
target_include_directories(TIME PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})


