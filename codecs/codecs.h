#ifndef PLAYERCODECS_H
#define PLAYERCODECS_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QMetaClassInfo>
#include <QDebug>

class Codec;

class Codecs : public QObject
{
    Q_OBJECT
public:
    static Codecs* instance();
    static void init();

    QList<QByteArray> codecs();

    Codec* createCodec(const QByteArray& mimetype);

    template<typename T>
    void addCodec();

private:
    Codecs(QObject* parent = 0);

    static Codecs* s_inst;

    QHash<QByteArray, QMetaObject> m_codecs;
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

#endif
