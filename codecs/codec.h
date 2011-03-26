#ifndef CODEC_H
#define CODEC_H

#include <QObject>
#include <QAudioFormat>
#include <QByteArray>

class Codec;

class AudioFileInformation : public QObject
{
    Q_OBJECT
public:
    AudioFileInformation(QObject* parent = 0);

    void setFilename(const QString& filename);
    QString filename() const;

    virtual int length() const = 0;

private:
    QString m_filename;
};

class Codec : public QObject
{
    Q_OBJECT
public:
    enum Status { Ok, NeedInput, Error };

    Codec(QObject* parent = 0);

    virtual bool init(const QAudioFormat& format) = 0;
    virtual void deinit() = 0;

signals:
    void output(QByteArray* data);
    void position(int position);

public slots:
    virtual void feed(const QByteArray& data, bool end = false) = 0;
    virtual Status decode() = 0;
};

#endif
