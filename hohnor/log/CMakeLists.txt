# Create static library .o library for upper level to use
AUX_SOURCE_DIRECTORY(./ DIR_SRCS)
add_library(LOG STATIC ${DIR_SRCS})

# Since using Current thread link to THREAD
target_link_libraries(LOG PUBLIC THREAD)
target_link_libraries(LOG PUBLIC TIME)
target_link_libraries(LOG PUBLIC FILE)

#add current directory *.h for upper level to include
target_include_directories(LOG INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})