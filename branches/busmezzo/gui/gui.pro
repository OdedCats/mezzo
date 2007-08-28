# QMAKE file for GUI project. Make changes in this file, NOT in the MAKEFILE

TEMPLATE = app
TARGET = mezzo_gui
DEFINES += _BUSES
INCLUDEPATH += $(QTDIR)/include $(QTDIR)/include/QtCore $(QTDIR)/include/QtGui $(QTDIR)/include/QtDesigner $(QTDIR)/include/QtNetwork $(QTDIR)/include/ActiveQt $(QTDIR)/include/Qt3Support

CONFIG(debug, debug|release) {
     LIBS +=  -L../mezzo_lib/Debug -lmezzo_lib -L../mezzoAnalyzer/Debug -lmezzoAnalyzer 
 } else {
     LIBS +=  -L../mezzo_lib/Release -lmezzo_lib -L../mezzoAnalyzer/Release -lmezzoAnalyzer 
 }

LIBS += -L$(QTDIR)/lib -lQtCore -lQtGui -lQtNetwork -lQt3Support -lQtDesigner 

QT+= core gui qt3support 
#activeqt xml network svg
CONFIG += uic4

# Input
HEADERS += canvas_qt4.h parametersdialog_qt4.h
FORMS += canvas_qt4.ui parametersdialog_qt4.ui
SOURCES += canvas_qt4.cpp main.cpp parametersdialog_qt4.cpp
RESOURCES += canvas_qt4.qrc
