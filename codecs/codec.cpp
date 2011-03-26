#include "codecs/codec.h"

AudioFileInformation::AudioFileInformation(int length)
    : m_length(length)
{
}

int AudioFileInformation::length() const
{
    return m_length;
}

Codec::Codec(QObject *parent)
    : QObject(parent)
{
}
