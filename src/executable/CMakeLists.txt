# CMakeLists for an executable

cmake_minimum_required(VERSION 3.14)
project(executable)

FetchContent_Declare(
	cpp-taskflow
	GIT_REPOSITORY https://github.com/cpp-taskflow/cpp-taskflow.git
	GIT_TAG v2.2.0
)
set(TF_BUILD_EXAMPLES FALSE)
set(TF_BUILD_TESTS FALSE)
set(TF_BUILD_BENCHMARKS FALSE)
FetchContent_MakeAvailable(cpp-taskflow)

set(SOURCE_FILES executable.cpp) # Set your source files here

include_directories(include ${cpp-taskflow_SOURCE_DIR})

add_executable(executable ${SOURCE_FILES})
install(TARGETS executable DESTINATION ${INSTALL_EXECUTABLE_DIR})