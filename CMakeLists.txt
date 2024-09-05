CMAKE_MINIMUM_REQUIRED(VERSION 3.23)

PROJECT(ryzenadjcli)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

add_subdirectory(lib)

include(CheckIPOSupported)
check_ipo_supported(RESULT has_ipo OUTPUT error)

add_executable(${PROJECT_NAME} argparse.c main.c)

if(has_ipo)
    message(STATUS "${PROJECT_NAME}: IPO / LTO enabled")
    set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if (WIN32)
    target_link_directories(${PROJECT_NAME} PRIVATE win32)
endif ()

include(GNUInstallDirs)
target_link_libraries(${PROJECT_NAME} RYADJ::RyzenAdjLib)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
