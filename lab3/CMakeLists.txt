cmake_minimum_required(VERSION 3.10)
project(RasterLabCpp)

set(CMAKE_CXX_STANDARD 17)

# Поиск пакетов GTKmm
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)

# Добавление исходного файла
add_executable(raster_app main.cpp)

# Линковка библиотек
target_include_directories(raster_app PRIVATE ${GTKMM_INCLUDE_DIRS})
target_link_libraries(raster_app PRIVATE ${GTKMM_LIBRARIES})