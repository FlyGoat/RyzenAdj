#cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

#define project name
PROJECT(ryzenadj)

set(CMAKE_CXX_STANDARD 11)
SET(LINK_DIR ./prebuilt)

INCLUDE_DIRECTORIES(${INC_DIR})
LINK_DIRECTORIES(${LINK_DIR})

AUX_SOURCE_DIRECTORY(./ SRC_DIR)

if(WIN32)
set(OS_SOURCE lib/winring0.cpp)
set(OS_LINK_LIBRARY WinRing0x64)
else()
set(OS_SOURCE lib/libpci.c)
#if (CMAKE_BUILD_TYPE STREQUAL "Release")
#Static link libpci in release build
#set(OS_LINK_LIBRARY libpci.a)
#else()
set(OS_LINK_LIBRARY pci)
#endif()
endif()

add_definitions(-D_LIBRYZENADJ_INTERNAL)

ADD_EXECUTABLE(${PROJECT_NAME} ${OS_SOURCE} lib/nb_smu_ops.c lib/api.c argparse.c main.c)
target_link_libraries(${PROJECT_NAME} ${OS_LINK_LIBRARY})
#SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)
ADD_LIBRARY (libryzenadj SHARED ${OS_SOURCE} lib/nb_smu_ops.c lib/api.c)
set_target_properties(libryzenadj PROPERTIES PREFIX "")
target_link_libraries(libryzenadj ${OS_LINK_LIBRARY})
#SET_TARGET_PROPERTIES(libryzenadj PROPERTIES LINKER_LANGUAGE C)
