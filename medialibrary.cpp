#include "medialibrary.h"
#include "io.h"
#include <QDebug>
#include <QDir>
#include <QTimer>
#include <QStack>
#include <QSqlDatabase>
#include <QSqlQuery>

Q_DECLARE_METATYPE(PathSet)
Q_DECLARE_METATYPE(Artist)

class MediaJob;

struct MediaState
{
    QString path;
    PathSet files;
    PathSet dirs;
};

struct MediaData
{
    MediaData();

    bool updatePaths(MediaJob* job);
    void readLibrary(MediaJob* job);

    void removeNonExistingFiles(MediaJob* job);

    void createTables();
    int addArtist(const QString& name, bool* added = 0);
    int addAlbum(int artistid, const QString& name, bool* added = 0);
    int addTrack(int artistid, int albumid, const QString& name, const QString& filename, int trackno, bool* added = 0);

    bool pushState(PathSet& paths, const QString& prefix);

    QSqlDatabase database;
    PathSet paths;
    QStack<MediaState> states;
};

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

    static void deinit();

signals:
    void tag(const Tag& tag);
    void tagWritten(const QString& filename);

    void artist(const Artist& artist);
    void trackRemoved(int trackid);
    void updateStarted();
    void updateFinished();

private:
    Q_INVOKABLE void startJob();

    void readTag(const QString& path, Tag& tag);

    void updatePaths(const PathSet& paths);
    void requestTag(const QString& filename);
    void setTag(const QString& filename, const Tag& tag);
    void readLibrary();

    void createData();

    friend class MediaData;

private slots:
    void updatePaths();

private:
    Type m_type;
    QVariant m_arg;

    static MediaData* s_data;
};

MediaData::MediaData()
{
    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName("player.db");
    database.open();

    QStringList tables = database.tables();
    if (!tables.contains(QLatin1String("artists"))
        || !tables.contains(QLatin1String("albums"))
        || !tables.contains(QLatin1String("tracks")))
        createTables();
}

void MediaData::createTables()
{
    QSqlQuery q(database);
    q.exec(QLatin1String("create table artists (id integer primary key autoincrement, artist text not null)"));
    q.exec(QLatin1String("create table albums (id integer primary key autoincrement, album text not null, artistid integer, foreign key(artistid) references artist(id))"));
    q.exec(QLatin1String("create table tracks (id integer primary key autoincrement, track text not null, filename text not null, trackno integer, artistid integer, albumid integer, foreign key(artistid) references artist(id), foreign key(albumid) references album(id))"));
}

int MediaData::addArtist(const QString &name, bool* added)
{
    QSqlQuery q(database);

    q.prepare("select artists.id from artists where artists.artist = ?");
    q.bindValue(0, name);
    if (q.exec() && q.next()) {
        if (added)
            *added = false;
        return q.value(0).toInt();
    }

    q.prepare("insert into artists (artist) values (?)");
    q.bindValue(0, name);
    if (!q.exec()) {
        if (added)
            *added = false;
        return -1;
    }

    if (added)
        *added = true;
    return q.lastInsertId().toInt();
}

int MediaData::addAlbum(int artistid, const QString &name, bool* added)
{
    if (artistid <= 0)
        return artistid;

    QSqlQuery q(database);

    q.prepare("select albums.id from albums where albums.album = ? and albums.artistid = ?");
    q.bindValue(0, name);
    q.bindValue(1, artistid);
    if (q.exec() && q.next()) {
        if (added)
            *added = false;
        return q.value(0).toInt();
    }

    q.prepare("insert into albums (album, artistid) values (?, ?)");
    q.bindValue(0, name);
    q.bindValue(1, artistid);
    if (!q.exec()) {
        if (added)
            *added = false;
        return -1;
    }

    if (added)
        *added = true;
    return q.lastInsertId().toInt();
}

int MediaData::addTrack(int artistid, int albumid, const QString &name, const QString &filename, int trackno, bool* added)
{
    if (artistid <= 0 || albumid <= 0)
        return qMin(artistid, albumid);

    QSqlQuery q(database);

    q.prepare("select tracks.id from tracks, albums where tracks.track = ? and tracks.albumid = ? and albums.id = tracks.albumid and albums.artistid = ?");
    q.bindValue(0, name);
    q.bindValue(1, albumid);
    q.bindValue(2, artistid);
    if (q.exec() && q.next()) {
        if (added)
            *added = false;
        return q.value(0).toInt();
    }

    q.prepare("insert into tracks (track, filename, trackno, artistid, albumid) values (?, ?, ?, ?, ?)");
    q.bindValue(0, name);
    q.bindValue(1, filename);
    q.bindValue(2, trackno);
    q.bindValue(3, artistid);
    q.bindValue(4, albumid);
    if (!q.exec()) {
        if (added)
            *added = false;
        return -1;
    }

    if (added)
        *added = true;
    return q.lastInsertId().toInt();
}

static QString takeFirst(PathSet& paths)
{
    PathSet::Iterator it = paths.begin();
    if (it == paths.end())
        return QString();
    QString path = *it;
    paths.erase(it);
    return path;
}

bool MediaData::pushState(PathSet &paths, const QString& prefix)
{
    QString path = takeFirst(paths);
    if (path.isEmpty())
        return false;

    if (!prefix.isEmpty())
        path = QDir::cleanPath(prefix + QLatin1String("/") + path);

    QDir dir(path);
    QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
    QStringList dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    MediaState state;
    state.path = path;
    state.files = PathSet::fromList(files);
    state.dirs = PathSet::fromList(dirs);
    states.push(state);

    return true;
}

bool MediaData::updatePaths(MediaJob* job)
{
    if (states.isEmpty())
        if (!pushState(paths, QString()))
            return false;

    MediaState& state = states.top();
    if (!state.files.isEmpty()) {
        QString file = QDir::cleanPath(state.path + QLatin1String("/") + takeFirst(state.files));

        Tag tag;
        job->readTag(file, tag);
        if (tag.isValid()) {
            bool added;
            int artistid = addArtist(tag.data(QLatin1String("artist")).toString());
            int albumid = addAlbum(artistid, tag.data(QLatin1String("album")).toString());
            int trackid = addTrack(artistid, albumid, tag.data(QLatin1String("title")).toString(), file, tag.data(QLatin1String("track")).toInt(), &added);

            if (added) {
                Artist artist;
                artist.id = artistid;
                artist.name = tag.data(QLatin1String("artist")).toString();

                Album album;
                album.id = albumid;
                album.name = tag.data(QLatin1String("album")).toString();

                Track track;
                track.id = trackid;
                track.name = tag.data(QLatin1String("title")).toString();
                track.filename = file;

                album.tracks[trackid] = track;
                artist.albums[albumid] = album;

                emit job->artist(artist);
            }
        }

        return true;
    } else if (!state.dirs.isEmpty()) {
        pushState(state.dirs, state.path);
        return true;
    } else {
        states.pop();
    }

    removeNonExistingFiles(job);

    return true;
}

void MediaData::removeNonExistingFiles(MediaJob* job)
{
    QSqlQuery query;
    QFileInfo file;
    QSet<int> trackpending, albumpending, artistpending;

    query.exec("select tracks.id, tracks.filename, tracks.albumid, tracks.artistid from tracks");
    while (query.next()) {
        file.setFile(query.value(1).toString());
        if (!file.exists()) {
            qDebug() << "want to remove" << query.value(1).toString() << "which has an id of" << query.value(0).toInt();
            // remove from database
            trackpending.insert(query.value(0).toInt());
            albumpending.insert(query.value(2).toInt());
            artistpending.insert(query.value(3).toInt());
        }
    }
    if (trackpending.isEmpty())
        return;

    foreach(int trackid, trackpending) {
        query.exec("delete from tracks where tracks.id=" + QString::number(trackid));
        emit job->trackRemoved(trackid);
    }
    foreach(int albumid, albumpending) {
        query.exec("select tracks.albumid from tracks where tracks.albumid=" + QString::number(albumid));
        if (!query.next())
            query.exec("delete from albums where albums.id=" + QString::number(albumid));
    }
    foreach(int artistid, artistpending) {
        query.exec("select albums.artistid from albums where albums.artistid=" + QString::number(artistid));
        if (!query.next())
            query.exec("delete from artists where artists.id=" + QString::number(artistid));
    }
}

void MediaData::readLibrary(MediaJob* job)
{
    QSqlQuery artistQuery, albumQuery, trackQuery;

    artistQuery.exec("select artists.id, artists.artist from artists");
    while (artistQuery.next()) {
        Artist artistData;

        int artistid = artistQuery.value(0).toInt();

        artistData.id = artistid;
        artistData.name = artistQuery.value(1).toString();

        albumQuery.prepare("select albums.id, albums.album from albums where albums.artistid = ?");
        albumQuery.bindValue(0, artistid);
        albumQuery.exec();
        while (albumQuery.next()) {
            Album albumData;

            int albumid = albumQuery.value(0).toInt();

            albumData.id = albumid;
            albumData.name = albumQuery.value(1).toString();

            trackQuery.prepare("select tracks.id, tracks.track, tracks.filename, tracks.trackno from tracks where tracks.artistid = ? and tracks.albumid = ? order by tracks.trackno");
            trackQuery.bindValue(0, artistid);
            trackQuery.bindValue(1, albumid);
            trackQuery.exec();

            while (trackQuery.next()) {
                Track trackData;

                trackData.id = trackQuery.value(0).toInt();
                trackData.name = trackQuery.value(1).toString();
                trackData.filename = trackQuery.value(2).toString();
                trackData.trackno = trackQuery.value(3).toInt();

                albumData.tracks[trackData.id] = trackData;
            }

            artistData.albums[albumData.id] = albumData;
        }

        emit job->artist(artistData);
        QThread::yieldCurrentThread();
    }
}

MediaData* MediaJob::s_data = 0;

void MediaJob::deinit()
{
    delete s_data;
}

MediaJob::MediaJob(QObject* parent)
    : IOJob(parent), m_type(None)
{
}

void MediaJob::createData()
{
    if (!s_data)
        s_data = new MediaData;
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
    emit updateStarted();

    createData();

    bool shouldUpdate = s_data->paths.isEmpty();
    s_data->paths += paths;

    if (shouldUpdate && !s_data->paths.isEmpty()) {
        QTimer::singleShot(0, this, SLOT(updatePaths()));
    } else
        emit updateFinished();
}

void MediaJob::updatePaths()
{
    if (s_data->updatePaths(this))
        QTimer::singleShot(0, this, SLOT(updatePaths()));
    else {
        emit updateFinished();
        emit finished();
    }
}

void MediaJob::requestTag(const QString &filename)
{
    Tag t(filename);
    emit tag(t);
    emit finished();
}

void MediaJob::setTag(const QString &filename, const Tag &tag)
{
    // ### need to write the tag here

    emit tagWritten(filename);
    emit finished();
}

void MediaJob::readLibrary()
{
    createData();
    s_data->readLibrary(this);
    emit finished();
}

void MediaJob::readTag(const QString &path, Tag& tag)
{
    tag = Tag(path);
}

#include "medialibrary.moc"

MediaLibrary* MediaLibrary::s_inst = 0;

MediaLibrary::MediaLibrary(QObject *parent) :
    QObject(parent)
{
    IO::instance()->registerJob<MediaJob>();
    connect(IO::instance(), SIGNAL(jobCreated(IOJob*)), this, SLOT(jobCreated(IOJob*)));

    qRegisterMetaType<PathSet>("PathSet");
    qRegisterMetaType<Tag>("Tag");
    qRegisterMetaType<Artist>("Artist");
}

MediaLibrary::~MediaLibrary()
{
    MediaJob::deinit();
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

        connect(media, SIGNAL(tag(Tag)), this, SIGNAL(tag(Tag)));
        connect(media, SIGNAL(artist(Artist)), this, SIGNAL(artist(Artist)));
        connect(media, SIGNAL(trackRemoved(int)), this, SIGNAL(trackRemoved(int)));
        connect(media, SIGNAL(tagWritten(QString)), this, SIGNAL(tagWritten(QString)));
        connect(media, SIGNAL(updateStarted()), this, SIGNAL(updateStarted()));
        connect(media, SIGNAL(updateFinished()), this, SIGNAL(updateFinished()));

        media->start();
    }
}
