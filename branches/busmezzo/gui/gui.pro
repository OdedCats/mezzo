# QMAKE file for GUI project. Make changes in this file, NOT in the MAKEFILE

TEMPLATE = app
TARGET = mezzo_gui
DEFINES += _BUSES
#$(QTDIR)/include/Qt3Support

CONFIG(debug, debug|release) {
     LIBS +=  -L../mezzo_lib/Debug -lmezzo_lib -L../mezzoAnalyzer/Debug -lmezzoAnalyzer 
     DEPENDS += ../mezzo_lib/Debug/mezzo_lib.lib ../mezzoAnalyzer/Debug/mezzoAnalyzer.lib
 } else {
     LIBS +=  -L../mezzo_lib/Release -lmezzo_lib -L../mezzoAnalyzer/Release -lmezzoAnalyzer 
 }

LIBS += -L$(QTDIR)/lib -lQtCore -lQtGui -lQtNetwork  -lQtDesigner 
#-lQt3Support

QT+= core gui 
#qt3support 
#activeqt xml network svg
CONFIG += uic4 embed_manifest_exe
#debug

# Input
HEADERS += canvas_qt4.h parametersdialog_qt4.h src/nodedlg.h src/batchrundlg.h src/outputview.h src/positionbackground.h
FORMS += canvas_qt4.ui parametersdialog_qt4.ui ui/nodedlg.ui ui/batchrundlg.ui ui/outputview.ui ui/positionbackground.ui
SOURCES += canvas_qt4.cpp main.cpp parametersdialog_qt4.cpp src/nodedlg.cpp src/batchrundlg.cpp src/outputview.cpp src/positionbackground.cpp
RESOURCES += canvas_qt4.qrc 
RC_FILE = mezzo.rc
DEPENDPATH += . ./src ../mezzo_lib/src
