#ifndef CODECDEVICE_H
#define CODECDEVICE_H

#include <QIODevice>
#include <QLinkedList>
#include "codecs/codecs.h"

class Buffer
{
public:
    Buffer();

    void add(QByteArray* sub);

    bool isEmpty() const;
    int size() const;
    QByteArray read(int size);

private:
    QLinkedList<QByteArray*> m_subs;
    int m_size;
};

class CodecDevice : public QIODevice
{
    Q_OBJECT
public:
    CodecDevice(QObject *parent = 0);
    ~CodecDevice();

    bool isSequential() const;
    qint64 bytesAvailable() const;

    void setInputDevice(QIODevice* input);
    void setCodec(Codec* codec);

protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

private slots:
    void codecOutput(QByteArray* output);

private:
    bool fillBuffer();

private:
    QIODevice* m_input;
    Codec* m_codec;

    Buffer m_decoded;
};

#endif // CODECDEVICE_H
