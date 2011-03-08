#ifndef PLAYERCODECS_H
#define PLAYERCODECS_H

#include <QObject>
#include <QStringList>
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QAudioFormat>

class Codec;
class CodecFactory;

class Codecs
{
public:
    static QStringList codecs();
    static Codec* create(const QString& codec);

    static void init();
    static void addCodec(const QString& codec, CodecFactory* factory);

private:
    Codecs();

    static QHash<QString, CodecFactory*> s_factories;
};

class Codec : public QObject
{
    Q_OBJECT
public:
    Codec(const QString& codec, QObject* parent = 0);

    QString codec() const;

    virtual bool init(const QAudioFormat& format) = 0;

signals:
    void needsInput();
    void output(const QByteArray& data);

public slots:
    virtual void feed(const QByteArray& data, bool end = false) = 0;
    virtual bool decode() = 0;

private:
    QString m_codec;
};

class CodecFactory
{
public:
    virtual Codec* create(const QString& codec) = 0;

protected:
    CodecFactory();
    virtual ~CodecFactory();
};

#endif
