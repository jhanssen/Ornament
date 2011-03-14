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

    MediaJob(QObject* parent = 0);

    Type type() const;
    void setType(Type type);

    QVariant arg() const;
    void setArg(const QVariant& arg);

protected:
    void init();

private:
    void updatePaths(const PathSet& paths);

private:
    Type m_type;
    QVariant m_arg;
};

MediaJob::MediaJob(QObject* parent)
    : IOJob(parent), m_type(None)
{
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

void MediaJob::init()
{
    switch (m_type) {
    case UpdatePaths:
        updatePaths(m_arg.value<PathSet>());
        break;
    default:
        break;
    }
}

void MediaJob::updatePaths(const PathSet &paths)
{
    foreach(QString path, paths) {
        qDebug() << "update path" << path;
    }
}

#include "medialibrary.moc"

MediaLibrary* MediaLibrary::s_inst = 0;

MediaLibrary::MediaLibrary(QObject *parent) :
    QObject(parent)
{
    IO::instance()->registerJob<MediaJob>();
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

void MediaLibrary::update()
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
    IO::instance()->postJob<MediaJob>(args);

    m_updatedPaths += toupdate;
}

void MediaLibrary::readLibrary()
{
}

void MediaLibrary::requestTag(const QString &filename)
{
}

void MediaLibrary::setTag(const QString &filename, const Tag &tag)
{
}