#ifndef PLAYERCODECS_H
#define PLAYERCODECS_H

#include <QObject>
#include <QStringList>
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QAudioFormat>
#include <QVariant>
#include <QMetaClassInfo>
#include <QDebug>

class Codec;
class CodecFactory;

class Codecs
{
public:
    static QList<QByteArray> codecs();

    static void init();

    static Codec* create(const QByteArray& mimetype);

    template<typename T>
    static void addCodec();

private:
    Codecs();

    static QHash<QByteArray, QMetaObject> s_codecs;
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

    Codec(QObject* parent = 0);

    virtual Tag* tag(const QString& filename) const;

    virtual bool init(const QAudioFormat& format) = 0;

signals:
    void output(QByteArray* data);

public slots:
    virtual void feed(const QByteArray& data, bool end = false) = 0;
    virtual Status decode() = 0;
};

template<typename T>
void Codecs::addCodec()
{
    QMetaObject metaobj = T::staticMetaObject;

    int mimepos = metaobj.indexOfClassInfo("mimetype");
    if (mimepos == -1) {
        qDebug() << "unable to create codec " << metaobj.className();
        return;
    }

    QMetaClassInfo mimeinfo = metaobj.classInfo(mimepos);
    QByteArray mimetype(mimeinfo.value());
    s_codecs[mimetype] = metaobj;
}

#endif
