cmake_minimum_required(VERSION 3.25)
project(szafir-host-proxy VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core DBus Widgets)
find_package(KF6CoreAddons REQUIRED)
find_package(KF6I18n REQUIRED)
find_package(KF6StatusNotifierItem REQUIRED)

qt_standard_project_setup()

add_executable(szafir-host-proxy
    main.cpp
    NativeHostIntegrator.h
    NativeHostIntegrator.cpp
    NativeMessagingService.h
    NativeMessagingService.cpp
)

target_link_libraries(szafir-host-proxy
    PRIVATE
    Qt6::Core
    Qt6::DBus
    Qt6::Widgets
    KF6::CoreAddons
    KF6::I18n
    KF6::StatusNotifierItem
)

install(TARGETS szafir-host-proxy DESTINATION bin)
install(FILES flatpak-host-wrapper DESTINATION share/szafir-host-proxy)
install(FILES pl.deno.kir.szafirhostproxy.service DESTINATION share/dbus-1/services)
install(FILES LICENSE DESTINATION share/licenses/pl.deno.kir.szafirhostproxy/szafir-host-proxy)
include(GNUInstallDirs)
set(KDE_INSTALL_LOCALEDIR "${CMAKE_INSTALL_DATAROOTDIR}/locale")
ki18n_install(translations)
