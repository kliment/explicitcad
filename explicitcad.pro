QT += core gui opengl widgets
CONFIG += qscintilla2
CONFIG += c++14

#macx {
#    QMAKE_POST_LINK = install_name_tool -change libqscintilla2_qt$${QT_MAJOR_VERSION}.13.dylib $$[QT_INSTALL_LIBS]/libqscintilla2_qt$${QT_MAJOR_VERSION}.13.dylib $(TARGET)
#}

HEADERS      = mainwindow.h backdrop.h glmesh.h mesh.h canvas.h loader.h preferences.h viewwidget.h
SOURCES      = main.cpp mainwindow.cpp backdrop.cpp glmesh.cpp mesh.cpp loader.cpp canvas.cpp preferences.cpp tab.cpp
RESOURCES    = explicitcad.qrc
RESOURCES += gl/gl.qrc


isEmpty(PREFIX) {
  PREFIX = /usr/local
}
target.path = $$PREFIX/bin/
INSTALLS += target
