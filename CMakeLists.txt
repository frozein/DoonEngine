cmake_minimum_required(VERSION 3.12)

project(doonengine C)

file(GLOB_RECURSE doonengine_src CONFIGURE_DEPENDS "src/*.c")
file(GLOB doonengine_lib CONFIGURE_DEPENDS "dependencies/lib/*.lib")

add_executable(${PROJECT_NAME} ${doonengine_src})
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "assets/")
target_link_libraries(${PROJECT_NAME} ${doonengine_lib})
include_directories("src/" "dependencies/include/")