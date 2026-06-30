set(CMAKE_AUTOMOC ON)

add_executable(server
    main.cpp
    Server.h
    Server.cpp
    PortScanner.h
    PortScanner.cpp
)

target_include_directories(server PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/common
)

target_link_libraries(server PRIVATE
    common
    Qt6::Core
    Qt6::Network
)
