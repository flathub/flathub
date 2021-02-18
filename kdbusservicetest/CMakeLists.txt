cmake_minimum_required(VERSION 3.5)

set(KF5_VERSION "5.78.0")
project(Test VERSION ${KF5_VERSION})

find_package(ECM 5.78.0  NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)

find_package(Qt5DBus REQUIRED NO_MODULE)
find_package(KF5DBusAddons REQUIRED NO_MODULE)

add_executable(kdbusservicetest kdbusservicetest.cpp)
target_link_libraries(kdbusservicetest Qt5::DBus KF5::DBusAddons)
install(TARGETS kdbusservicetest ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
