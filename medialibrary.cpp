#include "medialibrary.h"
#include "io.h"
#include <QDebug>

Q_DECLARE_METATYPE(PathSet)

class MediaJob : public IOJob
{
    Q_OBJECT

    Q_PROPERTY(Type type READ type WRITE setType)
    Q_PROPERTY(QVariant arg READ arg WRITE setArg)

    Q_ENUMS(Type)
public:
    enum Type { None, UpdatePaths, RequestTag, SetTag, ReadLibrary };

    Q_INVOKABLE MediaJob(QObject* parent = 0);

    Type type() const;
    void setType(Type type);

    QVariant arg() const;
    void setArg(const QVariant& arg);

    void start();

private:
    Q_INVOKABLE void startJob();

    void updatePaths(const PathSet& paths);
    void requestTag(const QString& filename);
    void setTag(const QString& filename, const Tag& tag);
    void readLibrary();

private:
    Type m_type;
    QVariant m_arg;
};

MediaJob::MediaJob(QObject* parent)
    : IOJob(parent), m_type(None)
{
}

void MediaJob::start()
{
    QMetaObject::invokeMethod(this, "startJob");
}

void MediaJob::startJob()
{
    switch (m_type) {
    case UpdatePaths:
        updatePaths(m_arg.value<PathSet>());
        break;
    case RequestTag:
        requestTag(m_arg.toString());
        break;
    case SetTag:
    {
        QList<QVariant> args = m_arg.toList();
        if (args.size() == 2)
            setTag(args.at(0).toString(), args.at(1).value<Tag>());
        break;
    }
    case ReadLibrary:
        readLibrary();
        break;
    default:
        break;
    }
}

MediaJob::Type MediaJob::type() const
{
    return m_type;
}

void MediaJob::setType(Type type)
{
    m_type = type;
}

QVariant MediaJob::arg() const
{
    return m_arg;
}

void MediaJob::setArg(const QVariant &arg)
{
    m_arg = arg;
}

void MediaJob::updatePaths(const PathSet &paths)
{
    foreach(QString path, paths) {
        qDebug() << "update path" << path;
    }
}

void MediaJob::requestTag(const QString &filename)
{
}

void MediaJob::setTag(const QString &filename, const Tag &tag)
{
}

void MediaJob::readLibrary()
{
}

#include "medialibrary.moc"

MediaLibrary* MediaLibrary::s_inst = 0;

MediaLibrary::MediaLibrary(QObject *parent) :
    QObject(parent)
{
    IO::instance()->registerJob<MediaJob>();
    connect(IO::instance(), SIGNAL(jobCreated(IOJob*)), this, SLOT(jobCreated(IOJob*)));

    qRegisterMetaType<PathSet>("PathSet");
}

MediaLibrary* MediaLibrary::instance()
{
    return s_inst;
}

void MediaLibrary::init(QObject *parent)
{
    if (!s_inst)
        s_inst = new MediaLibrary(parent);
}

QStringList MediaLibrary::paths()
{
    return m_paths;
}

void MediaLibrary::setPaths(const QStringList &paths)
{
    m_paths = paths;
}

void MediaLibrary::addPath(const QString &path)
{
    m_paths.append(path);
}

void MediaLibrary::incrementalUpdate()
{
    PathSet toupdate;

    QStringList::ConstIterator it = m_paths.begin();
    QStringList::ConstIterator itend = m_paths.end();
    while (it != itend) {
        if (!m_updatedPaths.contains(*it))
            toupdate.insert(*it);
        ++it;
    }

    PropertyHash args;
    args["type"] = MediaJob::UpdatePaths;
    args["arg"] = QVariant::fromValue<PathSet>(toupdate);

    int jobid = IO::instance()->postJob<MediaJob>(args);
    m_pendingJobs.insert(jobid);

    m_updatedPaths += toupdate;
}

void MediaLibrary::fullUpdate()
{
    m_updatedPaths = PathSet::fromList(m_paths);

    PropertyHash args;
    args["type"] = MediaJob::UpdatePaths;
    args["arg"] = QVariant::fromValue<PathSet>(m_updatedPaths);

    int jobid = IO::instance()->postJob<MediaJob>(args);
    m_pendingJobs.insert(jobid);
}

void MediaLibrary::readLibrary()
{
    PropertyHash args;
    args["type"] = MediaJob::ReadLibrary;

    int jobid = IO::instance()->postJob<MediaJob>(args);
    m_pendingJobs.insert(jobid);
}

void MediaLibrary::requestTag(const QString &filename)
{
    PropertyHash args;
    args["type"] = MediaJob::RequestTag;
    args["arg"] = filename;

    int jobid = IO::instance()->postJob<MediaJob>(args);
    m_pendingJobs.insert(jobid);
}

void MediaLibrary::setTag(const QString &filename, const Tag &tag)
{
    QList<QVariant> list;
    list.append(filename);
    list.append(QVariant::fromValue<Tag>(tag));

    PropertyHash args;
    args["type"] = MediaJob::SetTag;
    args["arg"] = list;

    int jobid = IO::instance()->postJob<MediaJob>(args);
    m_pendingJobs.insert(jobid);
}

void MediaLibrary::jobCreated(IOJob *job)
{
    if (m_pendingJobs.contains(job->jobNumber())) {
        m_pendingJobs.remove(job->jobNumber());
        MediaJob* media = static_cast<MediaJob*>(job);

        media->start();
    }
}
