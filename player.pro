######################################################################
# Automatically generated by qmake (2.01a) Mon Mar 7 12:13:00 2011
######################################################################

TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += . codecs/mad/taglib/taglib codecs/mad/taglib/taglib/toolkit codecs/mad/taglib/taglib/mpeg/id3v2

QT += multimedia declarative sql

# Input
HEADERS += codecs/codecs.h \
    codecs/mad/codec_mad.h \
    codecdevice.h \
    audiodevice.h \
    audioplayer.h \
    musicmodel.h

SOURCES += main.cpp \
    codecs/codecs.cpp \
    codecs/mad/codec_mad.cpp \
    codecdevice.cpp \
    audiodevice.cpp \
    audioplayer.cpp \
    musicmodel.cpp

LIBS += codecs/mad/mad/libmad.a -ltag

OTHER_FILES += \
    player.qml
