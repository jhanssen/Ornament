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
class Tag;
class IOJob;
class TagJob;

class Codecs : public QObject
{
    Q_OBJECT
public:
    static Codecs* instance();
    static void init();

    QList<QByteArray> codecs();

    Codec* createCodec(const QByteArray& mimetype);
    void requestTag(const QByteArray& mimetype, const QString& filename);

    template<typename T>
    void addCodec();

    template<typename T>
    void addTag();

signals:
    void tagReady(Tag* tag);

private slots:
    void jobAboutToStart(IOJob* job);

private:
    Codecs(QObject* parent = 0);

    static Codecs* s_inst;

    QHash<QByteArray, QMetaObject> m_codecs;
    QHash<QByteArray, QMetaObject> m_tags;

    QHash<int, Tag*> m_pendingTags;
};

class Tag : public QObject
{
    Q_OBJECT
public:
    virtual ~Tag();

    QString filename() const;

    virtual QList<QString> keys() const = 0;
    virtual QVariant data(const QString& key) const = 0;
    virtual void setData(const QString& key, const QVariant& data);

protected:
    // This method will be called from the IO thread
    // so make sure it doesn't touch any QObject things
    virtual void readTag() = 0;

protected:
    Tag(const QString& filename, QObject* parent = 0);

private:
    QString m_filename;

    friend class TagJob;
};

class Codec : public QObject
{
    Q_OBJECT
public:
    enum Status { Ok, NeedInput, Error };

    Codec(QObject* parent = 0);

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
    m_codecs[mimetype] = metaobj;
}

template<typename T>
void Codecs::addTag()
{
    QMetaObject metaobj = T::staticMetaObject;

    int mimepos = metaobj.indexOfClassInfo("mimetype");
    if (mimepos == -1) {
        qDebug() << "unable to create codec " << metaobj.className();
        return;
    }

    QMetaClassInfo mimeinfo = metaobj.classInfo(mimepos);
    QByteArray mimetype(mimeinfo.value());
    m_tags[mimetype] = metaobj;
}

#endif
