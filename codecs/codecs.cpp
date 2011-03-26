#include "codecs/codecs.h"
#include "codecs/codec.h"
#include "codecs/mad/codec_mad.h"
#include <QMutexLocker>

Codecs* Codecs::s_inst = 0;

Codecs::Codecs(QObject* parent)
    : QObject(parent)
{
    addCodec<CodecMad>();
    addAudioFileInformation<AudioFileInformationMad>();
}

QList<QByteArray> Codecs::codecs()
{
    return m_codecs.keys();
}

Codec* Codecs::createCodec(const QByteArray &codec)
{
    QMutexLocker locker(&m_mutex);

    if (!m_codecs.contains(codec))
        return 0;

    QObject* obj = m_codecs.value(codec).newInstance(Q_ARG(QObject*, 0));
    Codec* c;
    if (!obj || !(c = qobject_cast<Codec*>(obj))) {
        delete obj;
        return 0;
    }

    return c;
}

AudioFileInformation* Codecs::createAudioFileInformation(const QByteArray &codec)
{
    QMutexLocker locker(&m_mutex);

    if (!m_infos.contains(codec))
        return 0;

    QObject* obj = m_infos.value(codec).newInstance(Q_ARG(QObject*, 0));
    AudioFileInformation* a;
    if (!obj || !(a = qobject_cast<AudioFileInformation*>(obj))) {
        delete obj;
        return 0;
    }

    return a;
}

void Codecs::init()
{
    if (!s_inst)
        s_inst = new Codecs;
}

Codecs* Codecs::instance()
{
    return s_inst;
}
