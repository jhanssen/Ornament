######################################################################
# Automatically generated by qmake (2.01a) Mon Mar 7 12:13:00 2011
######################################################################

TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += . libs3/inc

mac {
    INCLUDEPATH += /opt/local/include
    LIBS += -L/opt/local/lib
}

QT += multimedia declarative sql network

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
    filereader.h \
    buffer.h \
    medialibrary_file.h \
    medialibrary.h \
    medialibrary_s3.h \
    s3reader.h \
    awsconfig.h \
    audioreader.h

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
    filereader.cpp \
    buffer.cpp \
    medialibrary_file.cpp \
    medialibrary.cpp \
    medialibrary_s3.cpp \
    s3reader.cpp \
    awsconfig.cpp \
    audioreader.cpp

LIBS += libs3/build/lib/libs3.a -lmad -ltag -lcurl -lxml2

OTHER_FILES += \
    player.qml \
    Button.qml
