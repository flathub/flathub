cmake_minimum_required(VERSION 3.10)
project(GlideApp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 (or Qt5 if preferred)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network LinguistTools)
if(NOT Qt6_FOUND)
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network LinguistTools)
endif()

# Enable Qt's MOC (Meta-Object Compiler)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Add executables
add_executable(server server.cpp file_transfer_server.cpp)
add_executable(client client.cpp)
add_executable(glide_gui 
    mainwindow.cpp 
    main.cpp
    resources.qrc
    file_transfer.cpp
    constants.h
    settingsdialog.cpp
)

# Add translations for glide_gui
set(TS_FILES
    translations/glide_ar.ts
    translations/glide_en.ts
    translations/glide_ru.ts
)

# Ensure the translations output directory exists
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/translations")

# Manually run lrelease for each .ts file to ensure .qm generation
# Explicitly set lrelease executable path based on previous successful find
set(LRELEASE_EXECUTABLE "/usr/bin/lrelease")
message(STATUS "Explicitly set lrelease executable: ${LRELEASE_EXECUTABLE}")

if(NOT EXISTS "${LRELEASE_EXECUTABLE}")
    message(FATAL_ERROR "Explicitly set lrelease executable not found at ${LRELEASE_EXECUTABLE}. Please ensure Qt Linguist tools are installed.")
endif()

foreach(TS_FILE ${TS_FILES})
    get_filename_component(TS_NAME ${TS_FILE} NAME_WE)
    set(QM_FILE "${CMAKE_BINARY_DIR}/translations/${TS_NAME}.qm")
    
    add_custom_command(
        OUTPUT ${QM_FILE}
        COMMAND ${LRELEASE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${TS_FILE} -qm ${QM_FILE}
        DEPENDS ${TS_FILE}
        COMMENT "Generating ${QM_FILE}"
    )
    list(APPEND QM_FILES ${QM_FILE})
endforeach()

add_custom_target(generate_qm_files ALL DEPENDS ${QM_FILES})
add_dependencies(glide_gui generate_qm_files)

# Link Qt libraries to GUI
if(Qt6_FOUND)
    target_link_libraries(glide_gui Qt6::Core Qt6::Widgets Qt6::Network)
else()
    target_link_libraries(Glide_gui Qt5::Core Qt5::Widgets Qt5::Network)
endif()

if(WIN32)
    # Windows-specific libraries
    target_link_libraries(server ws2_32 user32)
    target_link_libraries(client ws2_32 user32)
    
    # Add Windows-specific compile definitions
    target_compile_definitions(server PRIVATE WIN32_LEAN_AND_MEAN)
    target_compile_definitions(client PRIVATE WIN32_LEAN_AND_MEAN)
    target_compile_definitions(glide_gui PRIVATE WIN32_LEAN_AND_MEAN)
    
    # Windows-specific GUI libraries
    target_link_libraries(glide_gui user32 shell32 ws2_32)
    
else()
    # Linux/Unix - Find packages properly
    target_link_libraries(glide_gui pthread)
    find_package(X11 REQUIRED)
    find_package(PkgConfig REQUIRED)
    
    # Use pkg-config to find XTest
    pkg_check_modules(XTST REQUIRED xtst)
    
    # Server links (usually doesn't need XTest for input capture)
    target_link_libraries(server 
        ${X11_LIBRARIES}
        pthread
    )
    
    # Client links (needs XTest for input injection)
    target_link_libraries(client 
        ${X11_LIBRARIES}
        ${XTST_LIBRARIES}
        pthread
    )
    
    # Add include directories
    target_include_directories(server PRIVATE ${X11_INCLUDE_DIR})
    target_include_directories(client PRIVATE 
        ${X11_INCLUDE_DIR}
        ${XTST_INCLUDE_DIRS}
    )
    
    # Add compile flags from pkg-config
    target_compile_options(client PRIVATE ${XTST_CFLAGS_OTHER})
endif()

# Add debug information
set(CMAKE_BUILD_TYPE Debug)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(server PRIVATE -Wall -Wextra -g)
    target_compile_options(client PRIVATE -Wall -Wextra -g)
    target_compile_options(glide_gui PRIVATE -Wall -Wextra -g)
elseif(MSVC)
    target_compile_options(server PRIVATE /W4)
    target_compile_options(client PRIVATE /W4)
    target_compile_options(Glide_gui PRIVATE /W4)
endif()

# Install targets
install(TARGETS server client glide_gui
    RUNTIME DESTINATION bin
)

# Print some helpful information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
if(UNIX AND NOT APPLE)
    message(STATUS "X11 libraries: ${X11_LIBRARIES}")
    message(STATUS "XTest libraries: ${XTST_LIBRARIES}")
endif()
if(Qt6_FOUND)
    message(STATUS "Using Qt6")
else()
    message(STATUS "Using Qt5")
endif()

# Define build date and time for use in source code
string(TIMESTAMP CMAKE_BUILD_DATE "%Y-%m-%d" UTC)
string(TIMESTAMP CMAKE_BUILD_TIME "%H:%M:%S UTC" UTC)
add_compile_definitions(BUILD_DATETIME="${CMAKE_BUILD_DATE} ${CMAKE_BUILD_TIME}")
