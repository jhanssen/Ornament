#ifndef CODEC_H
#define CODEC_H

#include <QObject>
#include <QAudioFormat>
#include <QByteArray>

class Codec;

class AudioFileInformation
{
public:
    AudioFileInformation(int length);

    int length() const;

private:
    int m_length;
};

class Codec : public QObject
{
    Q_OBJECT
public:
    enum Status { Ok, NeedInput, Error };

    Codec(QObject* parent = 0);

    virtual bool init(const QAudioFormat& format) = 0;
    virtual void deinit() = 0;

    virtual AudioFileInformation fileInformation(const QString& filename) const = 0;

signals:
    void output(QByteArray* data);
    void position(int position);

public slots:
    virtual void feed(const QByteArray& data, bool end = false) = 0;
    virtual Status decode() = 0;
};

#endif
