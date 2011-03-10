#ifndef PLAYERCODECS_H
#define PLAYERCODECS_H

#include <QObject>
#include <QStringList>
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QAudioFormat>
#include <QVariant>

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

class Tag
{
public:
    Tag(const QString& filename);
    virtual ~Tag();

    QString filename() const;

    virtual QList<QString> keys() const = 0;
    virtual QVariant data(const QString& key) const = 0;
    virtual void setData(const QString& key, const QVariant& data);

private:
    QString m_filename;
};

class Codec : public QObject
{
    Q_OBJECT
public:
    enum Status { Ok, NeedInput, Error };

    Codec(const QString& codec, QObject* parent = 0);

    QString codec() const;
    virtual Tag* tag(const QString& filename) const;

    virtual bool init(const QAudioFormat& format) = 0;

signals:
    void output(QByteArray* data);

public slots:
    virtual void feed(const QByteArray& data, bool end = false) = 0;
    virtual Status decode() = 0;

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
