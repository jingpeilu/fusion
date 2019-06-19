cmake_minimum_required(VERSION 3.5)

# Fix behavior of CMAKE_CXX_STANDARD when targeting macOS.
if (POLICY CMP0025)
    cmake_policy(SET CMP0025 NEW)
endif ()


project(Fusion)

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include_directories(${Fusion_SOURCE_DIR}/opencl)


# add preprocessor variable into the project
# enable GLM experimental feature to use quat.
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
add_definitions(-D__CL_ENABLE_EXCEPTIONS)
#add_definitions(-Wignored-attributes)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wignored-attributes")

#provide the source path so we can open our .cl file at runtime
ADD_DEFINITIONS(-DCL_SOURCE_DIR="${Fusion_SOURCE_DIR}")


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -O0")

#add_compile_definitions(HAVE_GLES)

find_package(Pangolin 0.4 REQUIRED)
include_directories(${Pangolin_INCLUDE_DIRS})
set(MyLibList ${Pangolin_LIBRARIES})

find_package(Eigen3 3.3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIRS})

find_package(glm REQUIRED)
include_directories(${GLM_INCLUDE_DIRS})
link_directories(${GLM_LIBRARY_DIRS})
add_definitions(${GLM_DEFINITIONS})

find_package(OpenCL REQUIRED)
list(APPEND MyLibList ${OPENCL_LIBRARY})

find_package(OpenCV REQUIRED)
list(APPEND MyLibList ${OpenCV_LIBS})

find_package(realsense2 2 QUIET)
if(realsense2_FOUND)
    include_directories(${realsense2_INCLUDE_DIR})
    list(APPEND MyLibList ${realsense2_LIBRARY})
    add_definitions(-DREALSENSE)
endif()
#find_package(GLEW REQUIRED)
#include_directories(${GLEW_INCLUDE_DIRS})
#add_definitions(${GLEW_DEFINITIONS})


#find_package(GLFW3 REQUIRED)
#include_directories(${GLFW3_INCLUDE_DIRS})

include_directories(./Utilities/lodepng)

add_subdirectory(Utilities/lodepng)


add_executable(Fusion main.cpp fusion.cpp fusion.h renderEngine.cpp renderEngine.h utilities.h Utilities/image_utilities.h Utilities/image_utilities.cpp inputStream.cpp inputStream.h computeEngine.cpp computeEngine.h GPUBuffer.cpp GPUBuffer.h libs.h)

target_link_libraries(Fusion LODEPNG ${MyLibList})
