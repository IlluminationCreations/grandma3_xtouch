add_subdirectory(XTouchController)
add_subdirectory(XTouchLib)
add_subdirectory(TCPServer)
add_subdirectory(helpers)

add_executable(SERVER main.cpp)
add_executable(TEST tests/main.cpp)
target_link_libraries(SERVER XTOUCHCONTROLLER_LIB TCPSERVER_LIB HELPERS_LIB)
target_link_libraries(TEST XTOUCHCONTROLLER_LIB TCPSERVER_LIB HELPERS_LIB)

