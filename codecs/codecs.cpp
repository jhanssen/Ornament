#include "codecs/codecs.h"
#include "codecs/mad/codec_mad.h"
#include "io.h"
#include <QMutexLocker>

Codecs* Codecs::s_inst = 0;

class TagJob : public IOJob
{
    Q_OBJECT
public:
    Q_INVOKABLE TagJob(QObject* parent = 0);

    void setTagGenerator(TagGenerator* tag);

signals:
    void tagReady(const Tag& tag);

private:
    Q_INVOKABLE void generatorReceived();

private:
    TagGenerator* m_generator;
};

#include "codecs.moc"

TagJob::TagJob(QObject *parent)
    : IOJob(parent), m_generator(0)
{
}

void TagJob::setTagGenerator(TagGenerator *generator)
{
    m_generator = generator;

    QMetaObject::invokeMethod(this, "generatorReceived");
}

void TagJob::generatorReceived()
{
    Tag tag = m_generator->readTag();

    emit tagReady(tag);
    emit finished();
}

Tag::Tag()
{
}

QString Tag::filename() const
{
    return m_filename;
}


QVariant Tag::data(const QString &key) const
{
    if (!m_data.contains(key))
        return QVariant();

    return m_data.value(key);
}

QList<QString> Tag::keys() const
{
    return m_data.keys();
}

TagGenerator::TagGenerator(const QString &filename, QObject *parent)
    : QObject(parent), m_filename(filename)
{
}

Tag TagGenerator::createTag()
{
    return Tag();
}

Tag TagGenerator::createTag(const QString &filename, const QHash<QString, QVariant> &data)
{
    Tag t;
    t.m_data = data;
    t.m_filename = filename;
    return t;
}

QString TagGenerator::filename() const
{
    return m_filename;
}

Codecs::Codecs(QObject* parent)
    : QObject(parent)
{
    addCodec<CodecMad>();
    addTagGenerator<TagGeneratorMad>();

    IO::instance()->registerJob<TagJob>();
    connect(IO::instance(), SIGNAL(jobAboutToStart(IOJob*)), this, SLOT(jobAboutToStart(IOJob*)));
    connect(IO::instance(), SIGNAL(error(QString)), this, SLOT(ioError(QString)));

    qRegisterMetaType<Tag>("Tag");
}

QList<QByteArray> Codecs::codecs()
{
    return m_codecs.keys();
}

void Codecs::ioError(const QString &error)
{
    qDebug() << "codecs ioerror" << error;
}

void Codecs::requestTag(const QByteArray &mimetype, const QString &filename)
{
    if (!m_tags.contains(mimetype))
        return;

    QObject* obj = m_tags.value(mimetype).newInstance(Q_ARG(QString, filename), Q_ARG(QObject*, 0));
    TagGenerator* generator;
    if (!obj || !(generator = qobject_cast<TagGenerator*>(obj))) {
        delete obj;
        return;
    }

    QMutexLocker locker(&m_tagMutex);
    int jobid = IO::instance()->postJob<TagJob>(filename);
    m_pendingTags[jobid] = generator;
}

void Codecs::jobAboutToStart(IOJob *job)
{
    TagGenerator* generator;
    {
        QMutexLocker locker(&m_tagMutex);
        QHash<int, TagGenerator*>::Iterator it = m_pendingTags.find(job->jobNumber());
        if (it == m_pendingTags.end())
            return;

        generator = it.value();
        m_pendingTags.erase(it);
    }

    TagJob* tagjob = static_cast<TagJob*>(job); // not sure if doing qobject_cast<> here would be safe
    connect(tagjob, SIGNAL(tagReady(Tag)), this, SIGNAL(tagReady(Tag)));
    tagjob->setTagGenerator(generator);
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
