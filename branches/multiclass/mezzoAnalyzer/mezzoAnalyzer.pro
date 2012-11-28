######################################################################
# Automatically generated by qmake (2.01a) m� 30. jul 18:56:56 2007
######################################################################

TEMPLATE = lib
TARGET = mezzoAnalyzer
DEPENDPATH += . src ui
INCLUDEPATH += . $(QTDIR)/include $(QTDIR)/include/QtCore $(QTDIR)/include/QtGui $(QTDIR)/include/QtDesigner $(QTDIR)/include/QtNetwork $(QTDIR)/include/ActiveQt 
#$(QTDIR)/include/Qt3Support 
LIBS+= $(SUBLIBS) -L../mezzo_lib/Debug -lmezzo_lib -L$(QTDIR)/lib -lQtCore -lQtGui 
# -lQt3Support 
#-lQtDesigner -lQtNetwork
QT+= core gui 
#qt3support 
# activeqt xml network svg
QMAKE= $(QTDIR)/bin/qmake
CONFIG += uic4 staticlib 
#debug
win32 {
     QMAKE_LFLAGS += /LARGEADDRESSAWARE
	 QMAKE_CXXFLAGS += /MP
	 }

# Input
HEADERS += src/assist.h \
           src/odcheckerdlg.h \
           src/odtabledelegate.h 
#           src/odtablemodel.h
FORMS += ui/odcheckdlg.ui
SOURCES += src/odcheckerdlg.cpp \
           src/odtabledelegate.cpp \
#           src/odtablemodel.cpp
           
           
