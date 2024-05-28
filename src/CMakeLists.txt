add_library(XTOUCH_LIB x-touch.cpp)
add_executable(SERVER main.cpp)
target_link_libraries(SERVER XTOUCH_LIB)
