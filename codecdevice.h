#ifndef CODECDEVICE_H
#define CODECDEVICE_H

#include <QIODevice>
#include "codecs/codecs.h"

class CodecDevice : public QIODevice
{
    Q_OBJECT
public:
    CodecDevice(QObject *parent = 0);

    bool isSequential() const;
    qint64 bytesAvailable() const;

    void setInputDevice(QIODevice* input);
    void setCodec(Codec* codec);

protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

private slots:
    void codecOutput(const QByteArray& output);
    void codecNeedsInput();

private:
    void fillBuffer() const;

private:
    QIODevice* m_input;
    Codec* m_codec;

    QByteArray m_decoded;
};

#endif // CODECDEVICE_H
