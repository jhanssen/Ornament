#include "codecs/codecs.h"
#include "codecs/mad/codec_mad.h"

Codecs* Codecs::s_inst = 0;

Tag::Tag(const QString &filename, QObject* parent)
    : QObject(parent), m_filename(filename)
{
}

Tag::~Tag()
{
}

QString Tag::filename() const
{
    return m_filename;
}

void Tag::setData(const QString &key, const QVariant &data)
{
    Q_UNUSED(key)
    Q_UNUSED(data)
}

Codecs::Codecs(QObject* parent)
    : QObject(parent)
{
    addCodec<CodecMad>();
    addTag<TagMad>();
}

QList<QByteArray> Codecs::codecs()
{
    return m_codecs.keys();
}

void Codecs::requestTag(const QByteArray &mimetype, const QString &filename)
{
    if (!m_tags.contains(mimetype))
        return;

    QObject* obj = m_tags.value(mimetype).newInstance(Q_ARG(QString, filename), Q_ARG(QObject*, 0));
    Tag* t;
    if (!obj || !(t = qobject_cast<Tag*>(obj))) {
        delete obj;
        return;
    }

    // Need to post a TagJob here and call readTag() from the job
}

Codec* Codecs::createCodec(const QByteArray &codec)
{
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

void Codecs::init()
{
    if (!s_inst)
        s_inst = new Codecs;
}

Codecs* Codecs::instance()
{
    return s_inst;
}

Codec::Codec(QObject *parent)
    : QObject(parent)
{
}
