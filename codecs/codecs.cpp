#include "codecs/codecs.h"
#include "codecs/mad/codec_mad.h"

QHash<QString, CodecFactory*> Codecs::s_factories;

Codecs::Codecs()
{
}

QStringList Codecs::codecs()
{
    return s_factories.keys();
}

Codec* Codecs::create(const QString &codec)
{
    if (!s_factories.contains(codec))
        return 0;
    return s_factories.value(codec)->create(codec);
}

void Codecs::addCodec(const QString &codec, CodecFactory *factory)
{
    s_factories[codec] = factory;
}

void Codecs::init()
{
    addCodec(QLatin1String("audio/mp3"), new CodecFactoryMad);
}

Codec::Codec(const QString &codec, QObject *parent)
    : QObject(parent), m_codec(codec)
{
}

QString Codec::codec() const
{
    return m_codec;
}

CodecFactory::CodecFactory()
{
}

CodecFactory::~CodecFactory()
{
}
