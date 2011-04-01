#include "medialibrary_file.h"
#include "io.h"
#include "filereader.h"
#include "codecs/codecs.h"
#include "codecs/codec.h"
#include <QDebug>
#include <QDir>
#include <QTimer>
#include <QStack>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QFileDialog>

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
    void clearDatabase();
    int addArtist(const QString& name, bool* added = 0);
    int addAlbum(int artistid, const QString& name, bool* added = 0);
    int addTrack(int artistid, int albumid, const QString& name, const QString& filename, int trackno, int duration, bool* added = 0);

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
    enum Type { None, UpdatePaths, RequestTag, SetTag, ReadLibrary, Refresh };

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
    void databaseCleared();

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
    q.exec(QLatin1String("create table tracks (id integer primary key autoincrement, track text not null, filename text not null, trackno integer, duration integer, artistid integer, albumid integer, foreign key(artistid) references artist(id), foreign key(albumid) references album(id))"));
}

void MediaData::clearDatabase()
{
    QSqlQuery q(database);
    q.exec(QLatin1String("delete from artists"));
    q.exec(QLatin1String("delete from albums"));
    q.exec(QLatin1String("delete from tracks"));
}

int MediaData::addArtist(const QString &name, bool* added)
{
    QSqlQuery q(database);

    q.prepare("select artists.id from artists where artists.artist = ? collate nocase");
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

    q.prepare("select albums.id from albums where albums.album = ? and albums.artistid = ? collate nocase");
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

int MediaData::addTrack(int artistid, int albumid, const QString &name, const QString &filename, int trackno, int duration, bool* added)
{
    if (artistid <= 0 || albumid <= 0)
        return qMin(artistid, albumid);

    QSqlQuery q(database);

    q.prepare("select tracks.id from tracks, albums where tracks.track = ? and tracks.albumid = ? and albums.id = tracks.albumid and albums.artistid = ? collate nocase");
    q.bindValue(0, name);
    q.bindValue(1, albumid);
    q.bindValue(2, artistid);
    if (q.exec() && q.next()) {
        if (added)
            *added = false;
        return q.value(0).toInt();
    }

    q.prepare("insert into tracks (track, filename, trackno, artistid, albumid, duration) values (?, ?, ?, ?, ?, ?)");
    q.bindValue(0, name);
    q.bindValue(1, filename);
    q.bindValue(2, trackno);
    q.bindValue(3, artistid);
    q.bindValue(4, albumid);
    q.bindValue(5, duration);
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

        QByteArray mimetype = MediaLibraryFile::instance()->mimeType(file);
        if (mimetype.isEmpty())
            return true;

        Tag tag;
        job->readTag(file, tag);
        if (tag.isValid()) {
            int duration = 0;
            AudioFileInformation* info = Codecs::instance()->createAudioFileInformation(mimetype);
            if (info) {
                info->setFilename(file);
                duration = info->length();
                delete info;
            }

            bool added;
            int artistid = addArtist(tag.data(QLatin1String("artist")).toString());
            int albumid = addAlbum(artistid, tag.data(QLatin1String("album")).toString());
            int trackid = addTrack(artistid, albumid, tag.data(QLatin1String("title")).toString(), file, tag.data(QLatin1String("track")).toInt(), duration, &added);

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
                track.trackno = tag.data(QLatin1String("track")).toInt();
                track.duration = duration;
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

            trackQuery.prepare("select tracks.id, tracks.track, tracks.filename, tracks.trackno, tracks.duration from tracks where tracks.artistid = ? and tracks.albumid = ? order by tracks.trackno");
            trackQuery.bindValue(0, artistid);
            trackQuery.bindValue(1, albumid);
            trackQuery.exec();

            while (trackQuery.next()) {
                Track trackData;

                trackData.id = trackQuery.value(0).toInt();
                trackData.name = trackQuery.value(1).toString();
                trackData.trackno = trackQuery.value(3).toInt();
                trackData.duration = trackQuery.value(4).toInt();
                trackData.filename = trackQuery.value(2).toString();

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
    case Refresh:
        s_data->clearDatabase();
        emit databaseCleared();
        // fall through
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
        QTimer::singleShot(50, this, SLOT(updatePaths()));
    } else {
        emit updateFinished();
        emit finished();
    }
}

void MediaJob::updatePaths()
{
    if (s_data->updatePaths(this))
        QTimer::singleShot(50, this, SLOT(updatePaths()));
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

#include "medialibrary_file.moc"

MediaLibraryFile::MediaLibraryFile(QObject *parent) :
    MediaLibrary(parent)
{
    connect(IO::instance(), SIGNAL(jobReady(IOJob*)), this, SLOT(jobReady(IOJob*)));
    connect(IO::instance(), SIGNAL(jobFinished(IOJob*)), this, SLOT(jobFinished(IOJob*)));

    qRegisterMetaType<PathSet>("PathSet");
    qRegisterMetaType<Tag>("Tag");
    qRegisterMetaType<Artist>("Artist");
}

MediaLibraryFile::~MediaLibraryFile()
{
    MediaJob::deinit();
}

void MediaLibraryFile::setSettings(QSettings *settings)
{
    MediaLibrary::setSettings(settings);

    if (m_settings)
        m_paths = m_settings->value(QLatin1String("mediaPaths")).toStringList();
}

void MediaLibraryFile::syncSettings()
{
    if (m_settings) {
        m_settings->setValue(QLatin1String("mediaPaths"), m_paths);
        m_settings->sync();
    }
}

void MediaLibraryFile::init(QObject *parent)
{
    if (!s_inst)
        s_inst = new MediaLibraryFile(parent);
}

QStringList MediaLibraryFile::paths()
{
    return m_paths;
}

void MediaLibraryFile::setPaths(const QStringList &paths)
{
    m_paths = paths;
    syncSettings();
}

void MediaLibraryFile::addPath(const QString &path)
{
    m_paths.append(path);
    syncSettings();
}

void MediaLibraryFile::incrementalUpdate()
{
    PathSet toupdate;

    QStringList::ConstIterator it = m_paths.begin();
    QStringList::ConstIterator itend = m_paths.end();
    while (it != itend) {
        if (!m_updatedPaths.contains(*it))
            toupdate.insert(*it);
        ++it;
    }

    MediaJob* job = new MediaJob;
    job->setType(MediaJob::UpdatePaths);
    job->setArg(QVariant::fromValue<PathSet>(toupdate));

    IO::instance()->startJob(job);
    m_jobs.insert(job, IOPtr(job));

    m_updatedPaths += toupdate;
}

void MediaLibraryFile::fullUpdate()
{
    m_updatedPaths = PathSet::fromList(m_paths);

    MediaJob* job = new MediaJob;
    job->setType(MediaJob::UpdatePaths);
    job->setArg(QVariant::fromValue<PathSet>(m_updatedPaths));

    IO::instance()->startJob(job);
    m_jobs.insert(job, IOPtr(job));
}

void MediaLibraryFile::readLibrary()
{
    MediaJob* job = new MediaJob;
    job->setType(MediaJob::ReadLibrary);

    IO::instance()->startJob(job);
    m_jobs.insert(job, IOPtr(job));
}

void MediaLibraryFile::requestMetaData(const QString &filename)
{
    MediaJob* job = new MediaJob;
    job->setType(MediaJob::RequestTag);
    job->setArg(filename);

    IO::instance()->startJob(job);
    m_jobs.insert(job, IOPtr(job));
}

void MediaLibraryFile::requestArtwork(const QString &filename)
{
    m_pendingArtwork.insert(filename);
    requestMetaData(filename);
}

void MediaLibraryFile::refresh()
{
    m_updatedPaths = PathSet::fromList(m_paths);

    MediaJob* job = new MediaJob;
    job->setType(MediaJob::Refresh);
    job->setArg(QVariant::fromValue<PathSet>(m_updatedPaths));

    IO::instance()->startJob(job);
    m_jobs.insert(job, IOPtr(job));
}

void MediaLibraryFile::setTag(const QString &filename, const Tag &tag)
{
    QList<QVariant> list;
    list.append(filename);
    list.append(QVariant::fromValue<Tag>(tag));

    MediaJob* job = new MediaJob;
    job->setType(MediaJob::SetTag);
    job->setArg(list);

    IO::instance()->startJob(job);
    m_jobs.insert(job, IOPtr(job));
}

void MediaLibraryFile::tagReceived(const Tag &t)
{
    emit metaData(t);

    if (m_pendingArtwork.remove(t.filename()))
        processArtwork(t);
}

void MediaLibraryFile::processArtwork(const Tag &tag)
{
    QVariant picture = tag.data(QLatin1String("picture0"));
    if (picture.isValid()) {
        QImage img = picture.value<QImage>();
        if (!img.isNull()) {
            emit artwork(img);
            return;
        }
    }

    // Check the directory
    QFileInfo info(tag.filename());
    if (info.exists()) {
        QDir dir = info.absoluteDir();

        QStringList files = dir.entryList((QStringList() << "*.png" << "*.jpg" << "*.jpeg"), QDir::Files, QDir::Name);
        foreach(const QString& file, files) {
            QImage img(dir.absoluteFilePath(file));
            if (!img.isNull()) {
                emit artwork(img);
                return;
            }
        }
    }

    emit artwork(QImage());
}

void MediaLibraryFile::jobReady(IOJob *job)
{
    if (m_jobs.contains(job)) {
        MediaJob* media = static_cast<MediaJob*>(job);

        connect(media, SIGNAL(tag(Tag)), this, SLOT(tagReceived(Tag)));
        connect(media, SIGNAL(artist(Artist)), this, SIGNAL(artist(Artist)));
        connect(media, SIGNAL(trackRemoved(int)), this, SIGNAL(trackRemoved(int)));
        connect(media, SIGNAL(tagWritten(QString)), this, SIGNAL(tagWritten(QString)));
        connect(media, SIGNAL(updateStarted()), this, SIGNAL(updateStarted()));
        connect(media, SIGNAL(updateFinished()), this, SIGNAL(updateFinished()));
        connect(media, SIGNAL(databaseCleared()), this, SIGNAL(cleared()));

        media->start();
    }
}

void MediaLibraryFile::jobFinished(IOJob *job)
{
    m_jobs.remove(job);

    IOJob::deleteIfNeeded(job);
}

QByteArray MediaLibraryFile::mimeType(const QString &filename) const
{
    if (filename.isEmpty())
        return QByteArray();

    int extpos = filename.lastIndexOf(QLatin1Char('.'));
    if (extpos > 0) {
        QString ext = filename.mid(extpos);
        if (ext == QLatin1String(".mp3"))
            return QByteArray("audio/mp3");
    }

    return QByteArray();
}

QIODevice* MediaLibraryFile::deviceForFilename(const QString &filename)
{
    FileReader* reader = new FileReader;
    reader->setFilename(filename);
    return reader;
}

MediaModel::MediaModel(QObject *parent)
    : QAbstractListModel(parent)
{
    updateFromLibrary();

    QHash<int, QByteArray> roles;
    roles[Qt::UserRole + 4] = "mediaindex";
    roles[Qt::UserRole + 5] = "mediaitem";
    setRoleNames(roles);
}

Qt::ItemFlags MediaModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = QAbstractListModel::flags(index);
    f |= Qt::ItemIsEditable;
    return f;
}

int MediaModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_data.size();
}

QVariant MediaModel::data(const QModelIndex &index, int role) const
{
    if (index.parent().isValid() || index.column() != 0)
        return QVariant();

    if (role == Qt::UserRole + 4)
        return index.row();
    else if (role != Qt::DisplayRole && role != Qt::UserRole + 5)
        return QVariant();

    int row = index.row();
    if (row < 0 || row >= m_data.size())
        return QVariant();

    QString data = m_data.at(row);
    if (data.isEmpty())
        data = QLatin1String("<click to set path>");
    return data;
}

QVariant MediaModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QAbstractListModel::headerData(section, orientation, role);
}

bool MediaModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.parent().isValid() || index.column() != 0 || role != Qt::DisplayRole)
        return false;

    int row = index.row();
    if (row < 0 || row >= m_data.size())
        return false;

    m_data[row] = value.toString();
    emit dataChanged(index, index);

    MediaLibraryFile* libraryFile = qobject_cast<MediaLibraryFile*>(MediaLibrary::instance());
    if (!libraryFile)
        return true;

    libraryFile->setPaths(m_data);
    libraryFile->fullUpdate();

    return true;
}

bool MediaModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;

    if (row < 0 || row > m_data.size())
        return false;

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
        m_data.insert(row, QString());
    endInsertRows();

    return true;
}

bool MediaModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;

    if (row < 0 || row + count - 1 > m_data.size())
        return false;

    beginRemoveRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
        m_data.removeAt(row);
    endRemoveRows();

    MediaLibraryFile* libraryFile = qobject_cast<MediaLibraryFile*>(MediaLibrary::instance());
    if (!libraryFile)
        return true;

    libraryFile->setPaths(m_data);

    return true;
}

void MediaModel::addRow()
{
    QModelIndex idx;
    insertRows(rowCount(idx), 1, idx);
}

void MediaModel::removeRow(int row)
{
    removeRows(row, 1, QModelIndex());
}

void MediaModel::setPathInRow(int row)
{
    QString existing;
    if (row >= 0 && row < m_data.size())
        existing = m_data.at(row);

    QString path = QFileDialog::getExistingDirectory(0, QLatin1String("Select directory for inclusion"), existing);
    if (!path.isEmpty()) {
        QModelIndex idx = index(row);
        if (!idx.isValid())
            return;
        setData(idx, path, Qt::DisplayRole);
    }
}

void MediaModel::refreshMedia()
{
    MediaLibraryFile* libraryFile = qobject_cast<MediaLibraryFile*>(MediaLibrary::instance());
    if (!libraryFile)
        return;

    libraryFile->refresh();
}

void MediaModel::updateFromLibrary()
{
    MediaLibraryFile* libraryFile = qobject_cast<MediaLibraryFile*>(MediaLibrary::instance());
    if (!libraryFile)
        return;

    m_data = libraryFile->paths();
}
