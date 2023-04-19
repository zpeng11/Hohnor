# Create static library .o library for upper level to use
AUX_SOURCE_DIRECTORY(./ DIR_SRCS)
add_library(CORE STATIC ${DIR_SRCS})

target_link_libraries(CORE PRIVATE COMMON)
target_link_libraries(CORE PRIVATE TIME)
target_link_libraries(CORE PRIVATE IO)
target_link_libraries(CORE PRIVATE LOG)

#add current directory *.h for upper level to include
target_include_directories(CORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})