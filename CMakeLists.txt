cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

cmake_policy(SET CMP0072 NEW)

project(ExplicitCAD)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Core Gui Widgets OpenGL REQUIRED)
find_package(QScintilla REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

set(SRCS main.cpp mainwindow.cpp backdrop.cpp glmesh.cpp mesh.cpp loader.cpp canvas.cpp preferences.cpp tab.cpp)
set(RESOURCES explicitcad.qrc gl/gl.qrc)

add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SRCS} ${RESOURCES})
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Gui Qt5::Widgets Qt5::OpenGL QScintilla::QScintilla OpenGL::GL Threads::Threads)
set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Info.plist.in)

install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION . COMPONENT Runtime
  BUNDLE DESTINATION . COMPONENT Runtime
)
