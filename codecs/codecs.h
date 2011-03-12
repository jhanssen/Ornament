#ifndef PLAYERCODECS_H
#define PLAYERCODECS_H

#include "codecs/tag.h"
#include <QObject>
#include <QString>
#include <QByteArray>
#include <QHash>
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
