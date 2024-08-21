cmake_minimum_required(VERSION 3.5)
project(auto-ryzenadj)

set(CMAKE_CXX_STANDARD 17)

## dependencies 
# daemon
find_package(Boost REQUIRED COMPONENTS system filesystem regex)
include(FetchContent)
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)
# applet
find_package(Qt5 REQUIRED COMPONENTS Widgets)
find_package(Boost REQUIRED COMPONENTS system filesystem)
include_directories(${Boost_INCLUDE_DIRS})

## executables
# daemon
add_executable(auto-ryzenadjd src/daemon/main.cpp)
target_link_libraries(auto-ryzenadjd ${Boost_LIBRARIES})
target_link_libraries(auto-ryzenadjd tomlplusplus::tomlplusplus)
# cli
add_executable(auto-ryzenadjctl src/cli/main.cpp)
# applet
add_executable(auto-ryzenadj-appindicator src/applet/main.cpp)
target_link_libraries(auto-ryzenadj-appindicator Qt5::Widgets)
target_link_libraries(auto-ryzenadj-appindicator ${Boost_LIBRARIES})

