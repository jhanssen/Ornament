#include "codecs/codec.h"

AudioFileInformation::AudioFileInformation(QObject *parent)
    : QObject(parent)
{
}

void AudioFileInformation::setFilename(const QString &filename)
{
    m_filename = filename;
}

QString AudioFileInformation::filename() const
{
    return m_filename;
}

Codec::Codec(QObject *parent)
    : QObject(parent)
{
}
