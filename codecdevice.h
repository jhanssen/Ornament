#ifndef CODECDEVICE_H
#define CODECDEVICE_H

#include <QIODevice>
#include <QLinkedList>
#include "buffer.h"

class AudioReader;
class Codec;

class CodecDevice : public QIODevice
{
    Q_OBJECT
public:
    CodecDevice(QObject *parent = 0);
    ~CodecDevice();

    bool isSequential() const;
    qint64 bytesAvailable() const;

    void setInputReader(AudioReader* input);
    void setCodec(Codec* codec);

    bool open(OpenMode mode);

    void pauseReader();
    void resumeReader();

protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

private slots:
    void codecOutput(QByteArray* output);

private:
    bool fillBuffer();

private:
    AudioReader* m_input;
    Codec* m_codec;

    Buffer m_decoded;
};

#endif // CODECDEVICE_H
