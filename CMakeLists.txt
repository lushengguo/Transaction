cmake_minimum_required(VERSION 3.5)
project(transaction) 

set(CMAKE_CXX_STANDARD 20)

# include(FetchContent)
# FetchContent_Declare(
#   googletest
#   # Specify the commit you depend on and update it regularly.
#   URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
# )
# # For Windows: Prevent overriding the parent project's compiler/linker settings
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)

# add_subdirectory(test)

add_executable(main main.cc)
target_include_directories(main PRIVATE ${PROJECT_SOURCE_DIR}/include)