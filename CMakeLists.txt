#cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

#define project name
PROJECT(ryzenadj)

SET(CMAKE_CXX_FLAGS "-std=c++11")

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")  
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")  

SET(INC_DIR ./inc)
SET(LINK_DIR ./prebuilt)

INCLUDE_DIRECTORIES(${INC_DIR})
LINK_DIRECTORIES(${LINK_DIR})

AUX_SOURCE_DIRECTORY(./ SRC_DIR)

if(WIN32)
ADD_EXECUTABLE(${PROJECT_NAME} winring0.cpp nb_smu_ops.cpp argparse.cpp main.cpp)
target_link_libraries(${PROJECT_NAME} WinRing0x64)
else()
ADD_EXECUTABLE(${PROJECT_NAME} libpci.cpp nb_smu_ops.cpp argparse.cpp main.cpp)
target_link_libraries(${PROJECT_NAME} pci)
endif()