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
ADD_EXECUTABLE(${PROJECT_NAME} lib/winring0.cpp lib/nb_smu_ops.c lib/api.c argparse.c main.c)
target_link_libraries(${PROJECT_NAME} WinRing0x64)
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)
ADD_LIBRARY (libryzenadj SHARED lib/winring0.cpp lib/nb_smu_ops.c lib/api.c)
target_link_libraries(libryzenadj WinRing0x64)
SET_TARGET_PROPERTIES(libryzenadj PROPERTIES LINKER_LANGUAGE C)   
else()
ADD_EXECUTABLE(${PROJECT_NAME} lib/libpci.c lib/nb_smu_ops.c lib/api.c argparse.c main.c)
target_link_libraries(${PROJECT_NAME} pci)
endif()
