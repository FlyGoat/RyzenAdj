#cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

#define project name
PROJECT(ryzenadj VERSION 0.11.1)

set(CMAKE_CXX_STANDARD 11)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

configure_file (
  "${PROJECT_SOURCE_DIR}/lib/ryzenadj_version.h.in"
  "${PROJECT_SOURCE_DIR}/lib/ryzenadj_version.h"
)

INCLUDE_DIRECTORIES(${INC_DIR})

AUX_SOURCE_DIRECTORY(./ SRC_DIR)

if(WIN32)
set(OS_SOURCE lib/osdep_win32.cpp)
set(OS_LINK_LIBRARY WinRing0x64)
set(OS_LINK_DIR ./win32)
else()
set(OS_SOURCE lib/osdep_linux.c)
#if (CMAKE_BUILD_TYPE STREQUAL "Release")
#Static link libpci in release build
#set(OS_LINK_LIBRARY libpci.a)
#else()
set(OS_LINK_LIBRARY pci)
#endif()
endif()

LINK_DIRECTORIES(${OS_LINK_DIR})

set(COMMON_SOURCES lib/nb_smu_ops.c lib/api.c lib/cpuid.c)
add_definitions(-D_LIBRYZENADJ_INTERNAL)

ADD_EXECUTABLE(${PROJECT_NAME} ${OS_SOURCE} ${COMMON_SOURCES} argparse.c main.c)
target_link_libraries(${PROJECT_NAME} ${OS_LINK_LIBRARY})
#SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
ADD_LIBRARY (libryzenadj ${OS_SOURCE} ${COMMON_SOURCES})
set_target_properties(libryzenadj PROPERTIES PREFIX "")
target_link_libraries(libryzenadj ${OS_LINK_LIBRARY})
set_target_properties(libryzenadj PROPERTIES
                              VERSION ${PROJECT_VERSION}
                            SOVERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(libryzenadj PROPERTIES PUBLIC_HEADER "lib/ryzenadj.h;lib/ryzenadj_version.h;lib/nb_smu_ops.h;lib/ryzenadj_priv.h")

if (UNIX)
  include(GNUInstallDirs)
  install(TARGETS ${PROJECT_NAME} libryzenadj
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endif (UNIX)
