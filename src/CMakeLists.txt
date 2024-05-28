add_subdirectory(XTouchController)
add_subdirectory(XTouchLib)

add_executable(SERVER main.cpp)
target_link_libraries(SERVER XTOUCH_LIB XTOUCHCONTROLLER_LIB)
