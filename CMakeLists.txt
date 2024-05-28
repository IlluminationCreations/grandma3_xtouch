project(XTouch)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET(CMAKE_C_COMPILER "/usr/bin/clang++")
SET(CMAKE_CXX_COMPILER "/usr/bin/clang++")

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
   message(SEND_ERROR "In-source builds are not allowed.")
endif()

add_library(XTOUCH_LIB x-touch.cpp)
add_executable(SERVER main.cpp)
target_link_libraries(SERVER XTOUCH_LIB)