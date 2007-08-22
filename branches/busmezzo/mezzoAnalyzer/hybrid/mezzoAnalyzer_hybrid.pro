
TEMPLATE = lib
TARGET = mezzoAnalyzer
DEPENDPATH += . ../src ../ui
INCLUDEPATH += .. . $(QTDIR)/include $(QTDIR)/include/QtCore $(QTDIR)/include/QtGui $(QTDIR)/include/QtDesigner $(QTDIR)/include/QtNetwork $(QTDIR)/include/ActiveQt $(QTDIR)/include/Qt3Support 
CONFIG(debug, debug|release) {
     LIBS +=  -L../../mezzo_lib/hybrid/Debug -lmezzo_hybrid_lib  
 } else {
     LIBS +=  -L../../mezzo_lib/hybrid/Release -lmezzo_hybrid_lib  
 }

LIBS+= -L$(QTDIR)/lib -lQtCore -lQtGui  -lQt3Support 

QT+= core gui qt3support 

QMAKE= $(QTDIR)/bin/qmake
CONFIG += uic4 staticlib

# Input
HEADERS += ../src/assist.h \
           ../src/odcheckerdlg.h \
           ../src/odtabledelegate.h
FORMS += ../ui/odcheckdlg.ui
SOURCES += ../src/odcheckerdlg.cpp \
           ../src/odtabledelegate.cpp
           
           
