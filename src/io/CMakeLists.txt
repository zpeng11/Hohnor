# Create static library .o library for upper level to use
AUX_SOURCE_DIRECTORY(./ DIR_SRCS)
add_library(IO STATIC ${DIR_SRCS})

#add current directory *.h for upper level to include
target_include_directories(IO INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})