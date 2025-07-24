cmake_minimum_required(VERSION 3.15)
project(client LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включить автоматическую MOC обработку
set(CMAKE_AUTOMOC ON)

find_package(Qt5 5.14 REQUIRED COMPONENTS Core Network Widgets)

set(SOURCE_FILES
    src/main.cpp
    src/NetworkDiscovery.cpp
    src/ClientManager.cpp
    src/mainwindow.cpp
)

set(HEADER_FILES
    src/NetworkDiscovery.h
    src/ClientManager.h
    src/mainwindow.h
)

set(RESOURCE_FILES
    resources.qrc
)

add_executable(client
    ${SOURCE_FILES}
    ${HEADER_FILES}
    ${RESOURCE_ADDED}
)

qt5_add_resources(RESOURCE_ADDED ${RESOURCE_FILES})

target_include_directories(client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(client PRIVATE
    Qt5::Core
    Qt5::Network
    Qt5::Widgets
)
