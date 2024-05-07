cmake_minimum_required(VERSION 3.27)

add_executable(sample src/main.cpp)
target_include_directories(sample PRIVATE src/)
target_link_libraries(sample PRIVATE katengine::katengine)
target_compile_features(sample PRIVATE cxx_std_23)
