# Now simply link against gtest or gtest_main as needed. Eg
add_executable(atomicIntegral_test atomicIntegral_test.cc)
target_include_directories(atomicIntegral_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(atomicIntegral_test gtest_main)
add_test(NAME atomicIntegral_test COMMAND atomicIntegral_test)