#ifndef PLAYERCODECS_H
#define PLAYERCODECS_H

#include "codecs/tag.h"
#include <QObject>
#include <QStringList>
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QAudioFormat>
#include <QVariant>
#include <QMetaClassInfo>
#include <QMutex>
#include <QDebug>

class Codec;
class Tag;
class IOJob;
class TagJob;
class TagGenerator;

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
    void addTagGenerator();

signals:
    void tagReady(const Tag& tag);

private slots:
    void jobAboutToStart(IOJob* job);
    void ioError(const QString& error);

private:
    Codecs(QObject* parent = 0);

    static Codecs* s_inst;

    QHash<QByteArray, QMetaObject> m_codecs;
    QHash<QByteArray, QMetaObject> m_tags;

    QMutex m_tagMutex;
    QHash<int, TagGenerator*> m_pendingTags;
};

class TagGenerator : public QObject
{
    Q_OBJECT
public:
    TagGenerator(const QString& filename, QObject* parent = 0);

protected:
    // This method will be called from the IO thread
    // so make sure it doesn't touch any QObject things
    virtual Tag readTag() = 0;

    Tag createTag();
    Tag createTag(const QString& filename, const QHash<QString, QVariant>& data);
    QString filename() const;

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
void Codecs::addTagGenerator()
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
