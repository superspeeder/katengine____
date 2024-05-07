cmake_minimum_required(VERSION 3.27)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Stb REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem program_options regex)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)


add_library(katengine
        src/kat/engine.cpp
        src/kat/engine.hpp
        src/kat/utils.cpp
        src/kat/utils.hpp)

target_include_directories(katengine PUBLIC src/)
target_include_directories(katengine PRIVATE ${Stb_INCLUDE_DIR})
target_link_libraries(katengine PUBLIC glm::glm glfw Vulkan::Vulkan spdlog::spdlog Boost::boost ${Boost_LIBRARIES} nlohmann_json::nlohmann_json)
target_compile_features(katengine PUBLIC cxx_std_23)

add_library(katengine::katengine ALIAS katengine)