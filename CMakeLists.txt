cmake_minimum_required(VERSION 3.21)
project(TimetableCodex2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(APPLE)
    set(_qt_root "/Users/alexanderyang/Qt/6.9.3")
    foreach(_suffix macos clang_64)
        set(_qt_candidate "${_qt_root}/${_suffix}")
        if(EXISTS "${_qt_candidate}")
            list(APPEND CMAKE_PREFIX_PATH "${_qt_candidate}")
        endif()
    endforeach()
endif()

find_package(Qt6 6.9.3 COMPONENTS Widgets REQUIRED)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/sidebar.cpp
    src/homepage.cpp
    src/jsonmanager.cpp
    src/timetablepage.cpp
    src/taskspage.cpp
    src/settingspage.cpp
)

set(HEADERS
    include/mainwindow.h
    include/sidebar.h
    include/homepage.h
    include/jsonmanager.h
    include/models.h
    include/timetablepage.h
    include/taskspage.h
    include/settingspage.h
)

qt_add_resources(APP_RESOURCES resources/resources.qrc)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${APP_RESOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE include)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)

install(TARGETS ${PROJECT_NAME})
