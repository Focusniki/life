cmake_minimum_required(VERSION 3.10)
project(OS_Overview_Server VERSION 1.0.0)

# Включил автоматическую обработку Qt метаобъектов
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Настройка стандарта C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Настройка путей для исходных файлов
include_directories(include)
set(SOURCE_FILES
    src/Server.cpp
    src/ClientConnection.cpp
    main.cpp
    include/Server.h
    include/ClientConnection.h
)

# Поиск Qt5 компонентов
find_package(Qt5 COMPONENTS Core Network REQUIRED)

# Создание исполняемого файла
add_executable(os_overview_server ${SOURCE_FILES})

# Подключение Qt модулей
target_link_libraries(os_overview_server
    Qt5::Core
    Qt5::Network
)

# Настройки для установки
install(TARGETS os_overview_server
    RUNTIME DESTINATION bin
)

# Для Linux: создание .deb пакета и systemd службы
if(UNIX AND NOT APPLE)
    # Файл службы systemd
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/os_overview.service.in"
        "${CMAKE_CURRENT_BINARY_DIR}/os_overview.service"
        @ONLY
    )

    # Установка службы
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/os_overview.service"
        DESTINATION "/lib/systemd/system"
    )

    # Postinst скрипт для активации службы
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/postinst" "#!/bin/sh\nsystemctl daemon-reload\nsystemctl enable os_overview.service\nsystemctl start os_overview.service\n")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/postinst"
        DESTINATION "/tmp"
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    )

    # Настройка CPack
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "/tmp/postinst")
    set(CPACK_DEBIAN_PACKAGE_NAME "os-overview-server")
    set(CPACK_DEBIAN_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Your Name <your.email@example.com>")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a, libqt5network5, systemd")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "/tmp/preinst")

    include(CPack)


endif()
