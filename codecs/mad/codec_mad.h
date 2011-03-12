#ifndef PLAYERCODEC_MAD_H
#define PLAYERCODEC_MAD_H

#include "codecs/codecs.h"
#include "codecs/mad/mad/include/mad.h"

#include <QHash>
#include <QList>

class TagGeneratorMad : public TagGenerator
{
    Q_OBJECT

    Q_CLASSINFO("mimetype", "audio/mp3")
public:
    Q_INVOKABLE TagGeneratorMad(const QString& filename, QObject* parent = 0);

protected:
    Tag readTag();
};

class CodecMad : public Codec
{
    Q_OBJECT

    Q_CLASSINFO("mimetype", "audio/mp3")
public:
    Q_INVOKABLE CodecMad(QObject* parent = 0);

    bool init(const QAudioFormat &format);

public slots:
    void feed(const QByteArray &data, bool end = false);
    Status decode();

private:
    void decode16(QByteArray** out, char** outptr, char** outend, int* outsize);
    void decode24(QByteArray** out, char** outptr, char** outend, int* outsize);

private:
    QAudioFormat m_format;

    struct mad_stream m_stream;
    struct mad_frame m_frame;
    struct mad_synth m_synth;

    mad_timer_t m_timer;

    QByteArray m_data;
    unsigned char* m_buffer;

    void (CodecMad::*decodeFunc)(QByteArray** out, char** outptr, char** outend, int* outsize);
};

#endif
