#include "codecs/codecs.h"
#include "codecs/mad/codec_mad.h"
#include "io.h"

Codecs* Codecs::s_inst = 0;

class TagJob : public IOJob
{
    Q_OBJECT
public:
    TagJob(QObject* parent = 0);

    void setTag(Tag* tag);

signals:
    void tagReady(Tag* tag);

private:
    Q_INVOKABLE void tagReceived();

private:
    Tag* m_tag;
};

#include "codecs.moc"

TagJob::TagJob(QObject *parent)
    : IOJob(parent), m_tag(0)
{
}

void TagJob::setTag(Tag *tag)
{
    m_tag = tag;

    QMetaObject::invokeMethod(this, "tagReceived");
}

void TagJob::tagReceived()
{
    m_tag->readTag();

    emit tagReady(m_tag);
    emit finished();
}

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

    IO::instance()->registerJob<TagJob>();
    connect(IO::instance(), SIGNAL(jobAboutToStart(IOJob*)), this, SLOT(jobAboutToStart(IOJob*)));
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

    int jobid = IO::instance()->postJob<TagJob>(filename);
    m_pendingTags[jobid] = t;
}

void Codecs::jobAboutToStart(IOJob *job)
{
    QHash<int, Tag*>::Iterator it = m_pendingTags.find(job->jobNumber());
    if (it == m_pendingTags.end())
        return;

    Tag* t = it.value();
    m_pendingTags.erase(it);

    TagJob* tagjob = static_cast<TagJob*>(job); // not sure if doing qobject_cast<> here would be safe
    connect(tagjob, SIGNAL(tagReady(Tag*)), this, SIGNAL(tagReady(Tag*)));
    tagjob->setTag(t);
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
