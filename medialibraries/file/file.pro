######################################################################
# Automatically generated by qmake (2.01a) Sat Apr 2 19:03:52 2011
######################################################################

TEMPLATE = lib
TARGET = file
DEPENDPATH += .
INCLUDEPATH += . ../..
DESTDIR = ..

CONFIG += plugin

mac {
    INCLUDEPATH += /opt/local/include
    LIBS += -L/opt/local/lib
}

QT += sql

DEFINES += TAG_PUBLIC_CTOR

# Input
HEADERS += file.h ../../tag.h
SOURCES += file.cpp ../../tag.cpp

LIBS += -ltag