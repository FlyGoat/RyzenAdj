#cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

#define project name
PROJECT(ryzenadj)

set(CMAKE_CXX_STANDARD 11)


INCLUDE_DIRECTORIES(${INC_DIR})

AUX_SOURCE_DIRECTORY(./ SRC_DIR)

if(WIN32)
set(OS_SOURCE lib/osdep_win32.cpp)
set(OS_LINK_LIBRARY WinRing0x64)
set(OS_LINK_DIR ./win32)
else()
set(OS_SOURCE lib/libpci.c)
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
ADD_LIBRARY (libryzenadj SHARED ${OS_SOURCE} ${COMMON_SOURCES})
set_target_properties(libryzenadj PROPERTIES PREFIX "")
target_link_libraries(libryzenadj ${OS_LINK_LIBRARY})
#SET_TARGET_PROPERTIES(libryzenadj PROPERTIES LINKER_LANGUAGE C)
