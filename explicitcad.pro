QT += core gui opengl widgets
CONFIG      += qscintilla2


#macx {
#    QMAKE_POST_LINK = install_name_tool -change libqscintilla2_qt$${QT_MAJOR_VERSION}.13.dylib $$[QT_INSTALL_LIBS]/libqscintilla2_qt$${QT_MAJOR_VERSION}.13.dylib $(TARGET)
#}

HEADERS      = mainwindow.h backdrop.h glmesh.h mesh.h canvas.h loader.h process.hpp
SOURCES      = main.cpp mainwindow.cpp backdrop.cpp glmesh.cpp mesh.cpp loader.cpp canvas.cpp process.cpp process_unix.cpp process_win.cpp
RESOURCES    = explicitcad.qrc
RESOURCES += gl/gl.qrc


CONFIG += c++14
