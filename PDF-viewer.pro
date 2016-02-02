#-------------------------------------------------
#
# Project created by QtCreator 2016-01-25T16:43:33
#
#-------------------------------------------------

QT       += core gui
CONFIG   += c++11
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = PDF-viewer
TEMPLATE = app


SOURCES += main.cpp\
        densityviewer.cpp \
    densityviewerwindow.cpp \
    densitydata.cpp

HEADERS  += densityviewer.h \
    densityviewerwindow.h \
    densitydata.h



unix|win32: LIBS += -L$$PWD/hdf5/lib/ -lhdf5 -lhdf5_cpp -lhdf5_hl -lhdf5_hl_cpp -lhdf5_tools -lszip -lz

INCLUDEPATH += $$PWD/hdf5/include
DEPENDPATH += $$PWD/hdf5/include

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/hdf5/lib/libhdf5.lib \
                                    $$PWD/hdf5/lib/libhdf5_cpp.lib \
                                    $$PWD/hdf5/lib/libhdf5_hl.lib \
                                    $$PWD/hdf5/lib/libhdf5_hl_cpp.lib \
                                    $$PWD/hdf5/lib/libhdf5_tools.lib \
                                    $$PWD/hdf5/lib/libszip.lib \
                                    $$PWD/hdf5/lib/libz.lib
else:unix|win32-g++: PRE_TARGETDEPS += $$PWD/hdf5/lib/libhdf5.a \
                                        $$PWD/hdf5/lib/libhdf5_cpp.a \
                                        $$PWD/hdf5/lib/libhdf5_hl.a \
                                        $$PWD/hdf5/lib/libhdf5_hl_cpp.a \
                                        $$PWD/hdf5/lib/libhdf5_tools.a \
                                        $$PWD/hdf5/lib/libszip.a \
                                        $$PWD/hdf5/lib/libz.a
