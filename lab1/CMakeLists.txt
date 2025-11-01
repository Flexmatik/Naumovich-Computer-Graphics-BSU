cmake_minimum_required(VERSION 3.10)
project(ColorModelConverter)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenCV REQUIRED)

if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV not found. Install libopencv-dev or set OpenCV_DIR to your OpenCVConfig.cmake folder.")
endif()

# Provide a fallback include path commonly used on Linux systems where
# OpenCV headers are installed under /usr/include/opencv4.
# We prefer target-based commands but keep compatibility with older CMake.

include_directories(${OpenCV_INCLUDE_DIRS} /usr/include/opencv4)

add_executable(ColorModelConverter
    main.cpp
)

# Make OPENCV_LIBS robust across OpenCV versions / CMake variables
if(DEFINED OpenCV_LIBS)
    set(OPENCV_LIBS ${OpenCV_LIBS})
elseif(DEFINED OpenCV_LIBRARIES)
    set(OPENCV_LIBS ${OpenCV_LIBRARIES})
else()
    # last resort: try linking the imported OpenCV target (OpenCV::opencv) if available
    if(TARGET OpenCV::opencv)
        set(OPENCV_LIBS OpenCV::opencv)
    else()
        message(WARNING "Could not determine OpenCV link libraries variable; attempting to link with ${OpenCV_LIBS}")
        set(OPENCV_LIBS ${OpenCV_LIBS})
    endif()
endif()

target_include_directories(ColorModelConverter PRIVATE ${OpenCV_INCLUDE_DIRS} /usr/include/opencv4)
target_link_libraries(ColorModelConverter ${OPENCV_LIBS})