set(CMAKE_AUTOMOC ON)

find_package(Qt6Widgets REQUIRED)

# Проверяем наличие GUI исходников
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow.cpp AND
   EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow.h AND
   EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/PortSearchModel.cpp AND
   EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/PortSearchModel.h)
    
    message(STATUS "Building GUI client")
    
    add_executable(client
        main_gui.cpp
        MainWindow.h
        MainWindow.cpp
        PortSearchModel.h
        PortSearchModel.cpp
        SecureClient.h
        SecureClient.cpp
    )
    
    target_include_directories(client PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/common
    )
    
    target_link_libraries(client PRIVATE
        common
        Qt6::Core
        Qt6::Network
        Qt6::Gui
        Qt6::Widgets
    )
else()
    message(STATUS "Building console client")
    
    add_executable(client
        main_console.cpp
        SecureClient.h
        SecureClient.cpp
    )
    
    target_include_directories(client PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/common
    )
    
    target_link_libraries(client PRIVATE
        common
        Qt6::Core
        Qt6::Network
    )
endif()

if(WIN32)
    set_target_properties(client PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
