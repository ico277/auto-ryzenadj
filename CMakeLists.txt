cmake_minimum_required(VERSION 3.5)
project(auto-ryzenadj)

set(CMAKE_CXX_STANDARD 17)

## options for features
option(ENABLE_DAEMON "Build the daemon executable" true)
option(ENABLE_CLI "Build the CLI executable" true)
option(ENABLE_APPLET "Build the applet executable" false)
option(ENABLE_SYSTEMD "Add service files for systemd" false)
option(ENABLE_OPENRC "Add service files for openrc" false)
option(DEBUG "Build in debug mode" false)

if(DEBUG)
    add_definitions(-DDEBUG -g)
endif()

# ensure at least one option is enabled
if(NOT (ENABLE_DAEMON OR ENABLE_CLI OR ENABLE_APPLET))
    message(FATAL_ERROR "At least one of ENABLE_DAEMON, ENABLE_CLI, or ENABLE_APPLET must be enabled.")
endif()

# daemon
if(ENABLE_DAEMON)
    # dependencies 
    find_package(Boost REQUIRED COMPONENTS system filesystem regex)
    include(FetchContent)
    FetchContent_Declare(
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(tomlplusplus)
    # executable
    add_executable(auto-ryzenadjd src/daemon/main.cpp)
    target_link_libraries(auto-ryzenadjd ${Boost_LIBRARIES})
    target_link_libraries(auto-ryzenadjd tomlplusplus::tomlplusplus)
    # installation
    install(TARGETS auto-ryzenadjd
        RUNTIME DESTINATION bin
    )
endif()
# cli
if(ENABLE_CLI)
    # executable
    add_executable(auto-ryzenadjctl src/cli/main.cpp)
    # installation
    install(TARGETS auto-ryzenadjctl
        RUNTIME DESTINATION bin
    )
endif()
# applet
if(ENABLE_APPLET)
    # dependencies 
    find_package(Qt5 REQUIRED COMPONENTS Widgets)
    find_package(Boost REQUIRED COMPONENTS system filesystem)
    include_directories(${Boost_INCLUDE_DIRS})
    # executable
    add_executable(auto-ryzenadj-appindicator src/applet/main.cpp)
    target_link_libraries(auto-ryzenadj-appindicator Qt5::Widgets)
    target_link_libraries(auto-ryzenadj-appindicator ${Boost_LIBRARIES})
    # installation
    install(TARGETS auto-ryzenadj-appindicator
        RUNTIME DESTINATION bin
    )
endif()

if(ENABLE_SYSTEMD)
    # systemd service file
    set(SYSTEMD_SERVICE_INSTALL_DIR "/etc/systemd/system" CACHE PATH "Systemd service installation directory")
    install(FILES auto-ryzenadjd.service
        DESTINATION ${SYSTEMD_SERVICE_INSTALL_DIR}
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    )
endif()
if(ENABLE_OPENRC)
    # openrc service file
    set(OPENRC_SERVICE_INSTALL_DIR "/etc/init.d" CACHE PATH "OpenRC service installation directory")
    install(FILES auto-ryzenadjd-openrc
        DESTINATION ${OPENRC_SERVICE_INSTALL_DIR}
        RENAME auto-ryzenadjd
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
    )
endif()

# additional files
if(ENABLE_DAEMON)
    set(CONFIG_INSTALL_DIR "/etc/" CACHE PATH "Directory for installing config files")
    install(FILES auto-ryzenadj.conf.example
        DESTINATION ${CONFIG_INSTALL_DIR}
        #RENAME "auto-ryzenadj.conf"
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    )
endif()
if(ENABLE_APPLET)
    set(DESKTOP_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/applications")
    install(FILES auto-ryzenadj-applet.desktop
        DESTINATION ${DESKTOP_INSTALL_DIR}
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    )
endif()
