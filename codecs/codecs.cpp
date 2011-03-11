#include "codecs/codecs.h"
#include "codecs/mad/codec_mad.h"

QHash<QByteArray, QMetaObject> Codecs::s_codecs;

Tag::Tag(const QString &filename)
    : m_filename(filename)
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

Codecs::Codecs()
{
}

QList<QByteArray> Codecs::codecs()
{
    return s_codecs.keys();
}

Codec* Codecs::create(const QByteArray &codec)
{
    if (!s_codecs.contains(codec))
        return 0;
    QObject* obj = s_codecs.value(codec).newInstance(Q_ARG(QObject*, 0));
    Codec* c;
    if (!obj || !(c = qobject_cast<Codec*>(obj))) {
        delete obj;
        return 0;
    }
    return c;
}

void Codecs::init()
{
    if (!s_codecs.isEmpty())
        return;

    addCodec<CodecMad>();
}

Codec::Codec(QObject *parent)
    : QObject(parent)
{
}

Tag* Codec::tag(const QString &filename) const
{
    Q_UNUSED(filename)
    return 0;
}
