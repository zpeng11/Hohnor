# Create static library .o library for upper level to use
AUX_SOURCE_DIRECTORY(./ DIR_SRCS)
add_library(NET STATIC ${DIR_SRCS})

# Since using Current thread link to THREAD
target_link_libraries(NET PUBLIC THREAD)
target_link_libraries(NET PUBLIC FILE)
target_link_libraries(NET PUBLIC TIME)
target_link_libraries(NET PUBLIC LOG)
target_link_libraries(NET PUBLIC IO)

#add current directory *.h for upper level to include
target_include_directories(NET PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include/hohnor/net)