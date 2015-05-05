cmake_minimum_required(VERSION 2.8.12)

project(izieditor)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(Boost_USE_MULTITHREADED ON)
set(SFML_STATIC_LIBRARIES OFF)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Find the QtWidgets library
find_package(Qt5Widgets)

file(GLOB_RECURSE SRC src/*.cpp src/*.hpp src/*.inl)
file(GLOB_RECURSE UI_FILES src/interface/*.ui)
file(GLOB_RECURSE QRC_FILES src/interface/*.qrc)

include_directories("${PROJECT_SOURCE_DIR}/src")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

qt5_wrap_ui(UI_HEADERS ${UI_FILES})
qt5_add_resources(RESOURCE_FILES ${QRC_FILES})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

source_group(main REGULAR_EXPRESSION src/main\\.cpp)
source_group(interface REGULAR_EXPRESSION src/interface/[^/]+)
source_group(graphics REGULAR_EXPRESSION src/graphics/[^/]+)
source_group(components REGULAR_EXPRESSION src/components/[^/]+)
source_group(core REGULAR_EXPRESSION src/core/[^/]+)
source_group(scene REGULAR_EXPRESSION src/scene/[^/])

add_executable(izieditor ${SRC} ${UI_HEADERS} ${UI_FILES} ${RESOURCE_FILES})

find_package(SFML 2 REQUIRED system window graphics network audio)
if(SFML_FOUND)
  include_directories(${SFML_INCLUDE_DIR})
  target_link_libraries(izieditor ${SFML_LIBRARIES})
endif()

find_package(Boost REQUIRED COMPONENTS system filesystem)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
    add_definitions("-DHAS_BOOST")
    target_link_libraries(izieditor ${Boost_LIBRARIES})
endif()


target_link_libraries(izieditor Qt5::Widgets)