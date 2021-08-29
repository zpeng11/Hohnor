# Create static library .o library for upper level to use
AUX_SOURCE_DIRECTORY(./ DIR_SRCS)
add_library(FILE STATIC ${DIR_SRCS})

target_link_libraries(FILE PUBLIC LOG)
target_link_libraries(FILE PUBLIC TIME)

#add current directory *.h for upper level to include
target_include_directories(FILE INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})