cmake_minimum_required(VERSION 3.10)
project(ImageProcessingLab_Cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenCV REQUIRED)

if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV not found. Install libopencv-dev or set OpenCV_DIR to your OpenCVConfig.cmake folder.")
endif()

include_directories(${OpenCV_INCLUDE_DIRS} /usr/include/opencv4)

add_executable(ImageLab
    main.cpp
    Processing.cpp
)

target_include_directories(ImageLab PRIVATE ${OpenCV_INCLUDE_DIRS} /usr/include/opencv4)

# Handle different OpenCV CMake variable names
if(DEFINED OpenCV_LIBS)
    set(OPENCV_LIBS ${OpenCV_LIBS})
elseif(DEFINED OpenCV_LIBRARIES)
    set(OPENCV_LIBS ${OpenCV_LIBRARIES})
else()
    if(TARGET OpenCV::opencv)
        set(OPENCV_LIBS OpenCV::opencv)
    else()
        message(WARNING "Could not determine OpenCV link libraries; attempting ${OpenCV_LIBS}")
        set(OPENCV_LIBS ${OpenCV_LIBS})
    endif()
endif()

target_link_libraries(ImageLab ${OPENCV_LIBS})