######################################################################
# Automatically generated by qmake (2.01a) Mon Mar 7 12:13:00 2011
######################################################################

TEMPLATE = app
TARGET = Ornament
DEPENDPATH += .
INCLUDEPATH += . libs3/inc

mac {
    INCLUDEPATH += /opt/local/include
    LIBS += -L/opt/local/lib
}

QT += multimedia declarative sql network

RESOURCES = src.qrc

# Input
HEADERS += codecs/codecs.h \
    codecs/codec.h \
    codecs/mad/codec_mad.h \
    tag.h \
    codecdevice.h \
    audiodevice.h \
    audioplayer.h \
    musicmodel.h \
    io.h \
    buffer.h \
    medialibrary.h \
    mediareader.h

SOURCES += main.cpp \
    codecs/codecs.cpp \
    codecs/codec.cpp \
    codecs/mad/codec_mad.cpp \
    tag.cpp \
    codecdevice.cpp \
    audiodevice.cpp \
    audioplayer.cpp \
    musicmodel.cpp \
    io.cpp \
    buffer.cpp \
    medialibrary.cpp \
    mediareader.cpp

LIBS += -lmad -ltag

OTHER_FILES += \
    player.qml \
    Button.qml
