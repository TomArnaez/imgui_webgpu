add_executable(shader_test shader_test.cpp)

find_library(SLANG_LIB slang)

target_link_libraries(shader_test PRIVATE webgpu ${SLANG_LIB})
target_compile_features(shader_test PRIVATE cxx_std_23)