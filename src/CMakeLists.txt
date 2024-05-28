add_subdirectory(XTouchController)
add_subdirectory(XTouchLib)
add_subdirectory(TCPServer)
add_subdirectory(helpers)

add_executable(SERVER main.cpp)
target_link_libraries(SERVER XTOUCH_LIB XTOUCHCONTROLLER_LIB TCPSERVER_LIB HELPERS_LIB)
