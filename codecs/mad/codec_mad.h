#ifndef PLAYERCODEC_MAD_H
#define PLAYERCODEC_MAD_H

#include "codecs/codec.h"
#include "codecs/mad/mad/include/mad.h"

#include <QHash>
#include <QList>

class AudioFileInformationMad : public AudioFileInformation
{
    Q_OBJECT

    Q_CLASSINFO("mimetype", "audio/mp3")
public:
    Q_INVOKABLE AudioFileInformationMad(QObject* parent = 0);

    int length() const;
};

class CodecMad : public Codec
{
    Q_OBJECT

    Q_CLASSINFO("mimetype", "audio/mp3")
public:
    Q_INVOKABLE CodecMad(QObject* parent = 0);
    ~CodecMad();

    bool init(const QAudioFormat &format);
    void deinit();

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
