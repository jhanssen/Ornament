######################################################################
# Automatically generated by qmake (2.01a) Sat Apr 2 19:03:52 2011
######################################################################

TEMPLATE = lib
TARGET = s3
DEPENDPATH += .
INCLUDEPATH += . ../.. ../../libs3/inc
DESTDIR = ..

CONFIG += plugin

QT += network

mac {
    INCLUDEPATH += /opt/local/include
    LIBS += -L/opt/local/lib
}

# Input
HEADERS += s3.h awsconfig.h ../../tag.h
SOURCES += s3.cpp awsconfig.cpp ../../tag.cpp

LIBS += ../../libs3/build/lib/libs3.a -lxml2 -lcurl -ltag