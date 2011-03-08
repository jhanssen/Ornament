#ifndef PLAYERCODEC_MAD_H
#define PLAYERCODEC_MAD_H

#include "codecs/codecs.h"
#include "codecs/mad/lib/include/mad.h"

class CodecFactoryMad : public CodecFactory
{
public:
    Codec* create(const QString& codec);

private:
    friend class Codecs;

    CodecFactoryMad();
};

class CodecMad : public Codec
{
    Q_OBJECT
public:
    CodecMad(const QString& codec, QObject* parent = 0);

    bool init(const QAudioFormat &format);

public slots:
    void feed(const QByteArray &data, bool end = false);
    bool decode();

private:
    QAudioFormat m_format;

    struct mad_stream m_stream;
    struct mad_frame m_frame;
    struct mad_synth m_synth;

    mad_timer_t m_timer;

    QByteArray m_data;
    unsigned char* m_buffer;
};

#endif
