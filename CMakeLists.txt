cmake_minimum_required(VERSION 3.23)
project(secur_audit)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Включаем автоматический MOC для всех подпроектов
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Указываем куда класть готовые исполняемые файлы
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

option(BUILD_SERVER "Build server executable" ON)
option(BUILD_CLIENT "Build client executable" ON)

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601)
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
elseif(UNIX)
    add_compile_definitions(_GNU_SOURCE)
    if(APPLE)
        add_compile_definitions(__APPLE__)
    else()
        add_compile_definitions(__LINUX__)
    endif()
endif()
    
find_package(Qt6 REQUIRED COMPONENTS Core Network Gui Widgets)

message(STATUS "Qt6 version: ${Qt6_VERSION}")
message(STATUS "Qt6Widgets found: ${Qt6Widgets_FOUND}")
add_compile_definitions(HAVE_QT6)

add_subdirectory(common)

if(BUILD_SERVER)
    add_subdirectory(server)
endif()

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()
