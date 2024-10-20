

add_executable(ChatRoomClient ${CMAKE_CURRENT_SOURCE_DIR}/ChatClient.cpp)
add_executable(ChatRoomServer ${CMAKE_CURRENT_SOURCE_DIR}/ChatServer.cpp)

target_link_libraries(ChatRoomClient PUBLIC Hohnor)
target_link_libraries(ChatRoomServer PUBLIC Hohnor)