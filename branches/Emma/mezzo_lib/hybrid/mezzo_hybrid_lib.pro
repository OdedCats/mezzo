
TEMPLATE = lib
CONFIG += staticlib debug
TARGET = mezzo_hybrid_lib
DEPENDPATH += . ../src
INCLUDEPATH += $(QTDIR)/include
# $(QTDIR)/include/Qt3Support 
LIBS+=  -L$(QTDIR)/lib -lQtCore -lQtGui 
#-lQt3Support 
QT+= core gui 
#qt3support 
QMAKE= $(QTDIR)/bin/qmake
DEFINES += _MIME _VISSIMCOM

# Input
HEADERS += ../src/busline.h \
           ../src/eventlist.h \
           ../src/gettime.h \
           ../src/Graph.h \
           ../src/grid.h \
           ../src/icons.h \
           ../src/link.h \
           ../src/linktimes.h \
           ../src/MMath.h \
           ../src/network.h \
           ../src/node.h \
           ../src/od.h \
           ../src/parameters.h \
           ../src/pvm.h \
           ../src/q.h \
           ../src/Random.h \
           ../src/route.h \
           ../src/sdfunc.h \
           ../src/server.h \
           ../src/trafficsignal.h \
           ../src/signature.h \
           ../src/turning.h \
           ../src/vehicle.h \
           ../src/vissimcom.h \
           ../src/vtypes.h \
           ../src/Graph.cpp
SOURCES += ../src/busline.cpp \
           ../src/eventlist.cpp \
           ../src/Graph.cpp \
           ../src/grid.cpp \
           ../src/icons.cpp \
           ../src/link.cpp \
           ../src/linktimes.cpp \
           ../src/network.cpp \
           ../src/node.cpp \
           ../src/od.cpp \
           ../src/parameters.cpp \
           ../src/pvm.cpp \
           ../src/q.cpp \
           ../src/Random.cpp \
           ../src/route.cpp \
           ../src/sdfunc.cpp \
           ../src/server.cpp \
           ../src/trafficsignal.cpp \
           ../src/signature.cpp \
           ../src/turning.cpp \
           ../src/vehicle.cpp \
           ../src/vissimcom.cpp \
           ../src/vtypes.cpp
