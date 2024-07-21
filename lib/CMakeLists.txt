CMAKE_MINIMUM_REQUIRED(VERSION 3.23)

project(ryzenadj)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

include(CheckIPOSupported)
check_ipo_supported(RESULT has_ipo OUTPUT error)

set(PROJECT_SOURCES
    nb_smu_ops.c
    api.c
    cpuid.c
)

set(PUB_HEADERS
    ryzenadj.h
)

if (WIN32)
    set(LINK_LIBS WinRing0x64)

    list(APPEND PROJECT_SOURCES
        osdep_win32.cpp
    )

elseif (LINUX)
    set(LINK_LIBS pci)

    list(APPEND PROJECT_SOURCES
        osdep_linux.c
    )

else ()
    message(FATAL_ERROR "!Unsupported OS!")
endif ()

add_library(${PROJECT_NAME} ${PUB_HEADERS} ${PROJECT_SOURCES})
add_library("RYADJ::RyzenAdjLib" ALIAS ${PROJECT_NAME})

if(has_ipo)
    message(STATUS "${PROJECT_NAME}: IPO / LTO enabled")
    set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if (WIN32)
    target_link_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../win32")
endif ()

target_compile_definitions(${PROJECT_NAME} PUBLIC _LIBRYZENADJ_INTERNAL)
target_link_libraries(${PROJECT_NAME} ${LINK_LIBS})

target_sources(${PROJECT_NAME} PUBLIC
    FILE_SET rylib_headers
    TYPE HEADERS
    FILES ${PUB_HEADERS}
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    FILE_SET rylib_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
