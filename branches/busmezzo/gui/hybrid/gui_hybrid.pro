# QMAKE file for GUI project. Make changes in this file, NOT in the MAKEFILE

TEMPLATE = app
TARGET = mezzo_gui_hybrid
INCLUDEPATH += .. . $(QTDIR)/include $(QTDIR)/include/QtCore $(QTDIR)/include/QtGui $(QTDIR)/include/QtDesigner $(QTDIR)/include/QtNetwork $(QTDIR)/include/ActiveQt $(QTDIR)/include/Qt3Support
DEFINES += _VISSIMCOM -MIME
CONFIG(debug, debug|release) {
     LIBS +=  -L../../mezzo_lib/hybrid/Debug -lmezzo_hybrid_lib -L../../mezzoAnalyzer/hybrid/Debug -lmezzoAnalyzer 
 } else {
     LIBS +=  -L../../mezzo_lib/hybrid/Release -lmezzo_hybrid_lib -L../../mezzoAnalyzer/hybrid/Release -lmezzoAnalyzer 
 }

LIBS += -L$(QTDIR)/lib -lQtCore -lQtGui -lQtNetwork -lQt3Support -lQtDesigner 

QT+= core gui qt3support 
#activeqt xml network svg
CONFIG += uic4

# Input
HEADERS += ../canvas_qt4.h ../parametersdialog_qt4.h
FORMS += ../canvas_qt4.ui ../parametersdialog_qt4.ui
SOURCES += ../canvas_qt4.cpp ../main.cpp ../parametersdialog_qt4.cpp
RESOURCES += ../canvas_qt4.qrc
