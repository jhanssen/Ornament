#include "musicmodel.h"
#include <QDeclarativeComponent>
#include <QDebug>

class MusicModelArtist : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int id READ identifier)
    Q_PROPERTY(QString name READ artistName)

public:
    int identifier() const { return id; }
    QString artistName() const { return artist; }

    int id;
    QString artist;

    QHash<int, MusicModelAlbum*> albums;
    QHash<int, MusicModelTrack*> tracks;
};

class MusicModelAlbum : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int id READ identifier)
    Q_PROPERTY(QString name READ albumName)

public:
    int identifier() const { return id; }
    QString albumName() const { return album; }

    int id;
    QString album;

    MusicModelArtist* artist;
    QHash<int, MusicModelTrack*> tracks;
};

class MusicModelTrack
{
public:
    int pos;

    int id;
    QString track;
    QString filename;
    int trackno;
    int duration;

    MusicModelArtist* artist;
    MusicModelAlbum* album;
};

#include "musicmodel.moc"

static bool trackLessThan(const MusicModelTrack* t1, const MusicModelTrack* t2)
{
    int less = t1->artist->artist.compare(t2->artist->artist);
    if (less)
        return less < 0;

    less = t1->album->album.compare(t2->album->album);
    if (less)
        return less < 0;

    if (t1->trackno != t2->trackno)
        return t1->trackno < t2->trackno;
    return t1->track < t2->track;
}

MusicModel::MusicModel(QObject *parent)
    : QAbstractTableModel(parent), m_artist(0), m_album(0), m_artistEmpty(false), m_albumEmpty(false)
{
    connect(MediaLibrary::instance(), SIGNAL(artist(Artist)), this, SLOT(updateArtist(Artist)));
    connect(MediaLibrary::instance(), SIGNAL(trackRemoved(int)), this, SLOT(removeTrack(int)));
    connect(MediaLibrary::instance(), SIGNAL(cleared()), this, SLOT(clearData()));
    MediaLibrary::instance()->readLibrary();

    QHash<int, QByteArray> roles;
    roles[Qt::UserRole + 1] = "musicitem";
    roles[Qt::UserRole + 2] = "musicid";
    roles[Qt::UserRole + 3] = "musicindex";
    setRoleNames(roles);

    qmlRegisterType<MusicModelArtist>("MusicModelArtist", 1, 0, "MusicModelArtist");
    qmlRegisterType<MusicModelAlbum>("MusicModelAlbum", 1, 0, "MusicModelAlbum");
}

MusicModel::~MusicModel()
{
    qDeleteAll(m_artists);
    qDeleteAll(m_albums);
    qDeleteAll(m_tracks);
}

void MusicModel::clearData()
{
    qDeleteAll(m_artists);
    m_artists.clear();
    qDeleteAll(m_albums);
    m_albums.clear();
    qDeleteAll(m_tracks);
    m_tracks.clear();

    m_tracksFile.clear();
    m_tracksPos.clear();
    m_artist = 0;
    m_album = 0;
    m_artistEmpty = m_albumEmpty = false;

    reset();
}

void MusicModel::updateArtist(const Artist &artist)
{
    bool insertArtist = (!m_artist && !m_album);
    bool insertAlbum = (m_artist && !m_album);
    bool insertTrack = (m_artist && m_album);

    bool inserted = false;
    bool shouldReset = false;

    MusicModelArtist* currentArtist;
    if (!m_artists.contains(artist.id)) {
        currentArtist = new MusicModelArtist;
        m_artists[artist.id] = currentArtist;

        if (insertArtist) {
            beginInsertRows(QModelIndex(), m_artists.size(), m_artists.size());
            inserted = true;
        }
    } else {
        currentArtist = m_artists[artist.id];
        shouldReset = true;
    }

    currentArtist->id = artist.id;
    currentArtist->artist = artist.name;

    if (inserted && insertArtist)
        endInsertRows();

    MusicModelAlbum* currentAlbum;
    MusicModelTrack* currentTrack;

    int albumsOldSize = currentArtist->albums.size();
    int albumsInserted = 0;
    foreach(const Album& album, artist.albums) {
        if (!currentArtist->albums.contains(album.id)) {
            currentAlbum = new MusicModelAlbum;
            currentArtist->albums[album.id] = currentAlbum;
            // Assume the album is not in m_albums either
            m_albums[album.id] = currentAlbum;
            ++albumsInserted;
        } else {
            currentAlbum = currentArtist->albums[album.id];
            shouldReset = true;
        }

        currentAlbum->id = album.id;
        currentAlbum->album = album.name;
        currentAlbum->artist = currentArtist;

        foreach(const Track& track, album.tracks) {
            if (!currentAlbum->tracks.contains(track.id)) {
                currentTrack = new MusicModelTrack;
                currentAlbum->tracks[track.id] = currentTrack;
                currentArtist->tracks[track.id] = currentTrack;
                m_tracks[track.id] = currentTrack;
            } else
                currentTrack = currentAlbum->tracks[track.id];

            currentTrack->id = track.id;
            currentTrack->track = track.name;
            currentTrack->filename = track.filename;
            currentTrack->trackno = track.trackno;
            currentTrack->duration = track.duration;
            currentTrack->artist = currentArtist;
            currentTrack->album = currentAlbum;
        }

        if (insertTrack && m_album->id == currentAlbum->id && !album.tracks.isEmpty()) {
            buildTracks();
            shouldReset = true;
        }
    }

    if (shouldReset)
        reset();
    else if (insertAlbum && m_artist->id == currentArtist->id && albumsInserted > 0) {
        beginInsertRows(QModelIndex(), albumsOldSize, albumsOldSize + albumsInserted - 1);
        endInsertRows();
    }
}

void MusicModel::removeTrack(int trackid)
{
    bool removeArtist = (!m_artist && !m_album);
    bool removeAlbum = (m_artist && !m_album);
    bool removeTrack = (m_artist && m_album);

    // Test and remove track
    QHash<int, MusicModelTrack*>::iterator tit = m_tracks.find(trackid);
    if (tit == m_tracks.end())
        return;

    MusicModelTrack* track = tit.value();

    if (removeTrack) {
        int trackIdx = m_tracksPos.indexOf(track);
        if (trackIdx >= 0) {
            beginRemoveRows(QModelIndex(), trackIdx, trackIdx);

            m_tracksPos.removeAt(trackIdx);
            QHash<QString, MusicModelTrack*>::iterator fit = m_tracksFile.find(track->filename);
            if (fit != m_tracksFile.end())
                m_tracksFile.erase(fit);
        }
    }

    m_tracks.erase(tit);

    // Remove track from album
    MusicModelAlbum* album = track->album;
    tit = album->tracks.find(trackid);
    if (tit == album->tracks.end())
        return;

    album->tracks.erase(tit);

    // Remove track from artist
    MusicModelArtist* artist = album->artist;
    tit = artist->tracks.find(trackid);
    if (tit == artist->tracks.end())
        return;

    artist->tracks.erase(tit);

    if (removeTrack)
        endRemoveRows();

    // Check if this was the last track of the album
    if (!album->tracks.isEmpty()) {
        // This should never be the currently played track so I'm not considering that here.
        delete track;
        return;
    }

    // Test and remove album
    QHash<int, MusicModelAlbum*>::iterator alit = m_albums.find(album->id);
    if (alit == m_albums.end())
        return;

    if (removeAlbum) {
        int albumIdx = m_albums.values().indexOf(album);
        if (albumIdx >= 0)
            beginRemoveRows(QModelIndex(), albumIdx, albumIdx);
    }

    m_albums.erase(alit);

    // Remove album from artist
    alit = artist->albums.find(album->id);
    if (alit == artist->albums.end())
        return;

    artist->albums.erase(alit);

    if (removeAlbum)
        endRemoveRows();

    // Check if this was the last album of the artist
    if (!artist->albums.isEmpty() && !artist->tracks.isEmpty()) {
        delete track;
        if (album == m_album)
            setCurrentAlbumId(-1);
        delete album;
        return;
    }

    // Test and remove artist
    QHash<int, MusicModelArtist*>::iterator arit = m_artists.find(artist->id);
    if (arit == m_artists.end())
        return;

    if (removeArtist) {
        int artistIdx = m_artists.values().indexOf(artist);
        if (artistIdx >= 0)
            beginRemoveRows(QModelIndex(), artistIdx, artistIdx);
    }

    m_artists.erase(arit);

    if (removeArtist)
        endRemoveRows();

    delete track;
    if (artist == m_artist)
        setCurrentArtistId(-1);
    else if (album == m_album)
        setCurrentAlbumId(-1);
    delete album;
    delete artist;
}

MusicModelArtist* MusicModel::currentArtist() const
{
    return m_artist;
}

int MusicModel::currentArtistId() const
{
    if (m_artist)
        return m_artist->id;
    return m_artistEmpty ? 0 : -1;
}

void MusicModel::setCurrentArtistId(int artist)
{
    MusicModelArtist* oldartist = m_artist;
    MusicModelAlbum* oldalbum = m_album;
    bool oldEmpty = m_artistEmpty;

    m_tracksPos.clear();
    m_tracksFile.clear();

    if (artist > 0) {
        m_artistEmpty = false;
        if (m_artists.contains(artist))
            m_artist = m_artists[artist];
        else
            m_artist = 0;
    } else {
        m_artistEmpty = (artist == 0);
        m_artist = 0;
    }
    m_albumEmpty = false;
    m_album = 0;

    if (m_artistEmpty)
        buildTracks();

    if (m_artist != oldartist || m_album != oldalbum || m_artistEmpty != oldEmpty)
        reset();
}

MusicModelAlbum* MusicModel::currentAlbum() const
{
    return m_album;
}

int MusicModel::currentAlbumId() const
{
    if (m_album)
        return m_album->id;
    return m_albumEmpty ? 0 : -1;
}

void MusicModel::setCurrentAlbumId(int album)
{
    if (m_artist == 0)
        return;

    MusicModelAlbum* oldalbum = m_album;
    bool oldEmpty = m_albumEmpty;

    m_tracksPos.clear();
    m_tracksFile.clear();

    if (album > 0) {
        m_albumEmpty = false;
        if (m_artist->albums.contains(album)) {
            m_album = m_artist->albums[album];

            buildTracks();
            reset();
            oldalbum = m_album; // prevent the reset() further down in this function
        } else
            m_album = 0;
    } else {
        m_albumEmpty = (album == 0);
        m_album = 0;
    }

    if (m_albumEmpty)
        buildTracks();

    if (m_album != oldalbum || m_albumEmpty != oldEmpty)
        reset();
}

void MusicModel::buildTracks()
{
    if (!m_artist && !m_artistEmpty)
        return;
    if (!m_album && !m_albumEmpty && !m_artistEmpty)
        return;

    QHash<int, MusicModelTrack*>::ConstIterator it;
    QHash<int, MusicModelTrack*>::ConstIterator itend;
    if (m_album) {
        it = m_album->tracks.begin();
        itend = m_album->tracks.end();
    } else if (m_artist) {
        it = m_artist->tracks.begin();
        itend = m_artist->tracks.end();
    } else {
        it = m_tracks.begin();
        itend = m_tracks.end();
    }

    m_tracksPos.clear();
    m_tracksFile.clear();

    while (it != itend) {
        m_tracksPos.append(*it);
        m_tracksFile[(*it)->filename] = *it;

        ++it;
    }

    // Sort by artist, album, track number, track (in that order)
    qSort(m_tracksPos.begin(), m_tracksPos.end(), trackLessThan);

    // Initialize the pos(ition) variable
    int pos = 0;
    foreach(MusicModelTrack* track, m_tracksPos) {
        track->pos = pos++;
    }
}

QVariant MusicModel::musicData(const QModelIndex &index, int role, bool hasAllEntry) const
{
    QVariant ret;

    // ### This is not really optimal.
    // ### Better to use keep both a hashmap and a list and keep them synchronized probably

    int row = index.row() - (hasAllEntry ? 1 : 0);

    if (role == Qt::DisplayRole) {
        if (!m_artist && !m_artistEmpty) {
            QList<MusicModelArtist*> artists = m_artists.values();

            if (row < artists.size()) {
                if (index.column() == 0)
                    ret = artists.at(row)->artist;
                else if (index.column() == 1)
                    ret = artists.at(row)->id;
            }

            return ret;
        } else if (m_artist && !m_album && !m_albumEmpty) {
            QList<MusicModelAlbum*> albums = m_artist->albums.values();

            if (row < albums.size()) {
                if (index.column() == 0)
                    ret = albums.at(row)->album;
                else if (index.column() == 1)
                    ret = albums.at(row)->id;
            }
            return ret;
        } else {
            if (row < m_tracksPos.size()) {
                if (index.column() == 0)
                    ret = m_tracksPos.at(row)->track;
                else if (index.column() == 1)
                    ret = m_tracksPos.at(row)->id;
            }
            return ret;
         }
    }
    return ret;
}

int MusicModel::columnCount(const QModelIndex &parent) const
{
    if (parent != QModelIndex())
        return 0;
    return 3;
}

int MusicModel::rowCount(const QModelIndex &parent) const
{
    if (parent != QModelIndex())
        return 0;
    if (!m_artist && !m_artistEmpty)
        return m_artists.size() + 1;
    else if (m_artist && !m_album && !m_albumEmpty)
        return m_artist->albums.size() + 1;
    return m_tracksPos.size();
}

QVariant MusicModel::data(const QModelIndex &index, int role) const
{
    bool hasAllEntry = ((!m_artist && !m_artistEmpty) || (m_artist && !m_album && !m_albumEmpty));
    if (hasAllEntry && index.row() == 0) {
        QVariant ret;

        if (role == Qt::DisplayRole || role == Qt::UserRole + 1) {
            if (index.column() == 0)
                ret = QLatin1String("All tracks");
            else if (index.column() == 1)
                ret = 0;
        } else if (role == Qt::UserRole + 2 || role == Qt::UserRole + 3)
            ret = 0;

        return ret;
    }

    if (role < Qt::UserRole)
        return musicData(index, role, hasAllEntry);

    if (role == Qt::UserRole + 3)
        return index.row();

    int col = role - Qt::UserRole - 1;
    QModelIndex idx = this->index(index.row(), col);

    return musicData(idx, Qt::DisplayRole, hasAllEntry);
}

QVariant MusicModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QAbstractTableModel::headerData(section, orientation, role);
}

QString MusicModel::filenameById(int id) const
{
    if (!m_tracks.contains(id))
        return QString();

    return m_tracks.value(id)->filename;
}

QString MusicModel::filenameByPosition(int position) const
{
    if (position < 0 || position >= m_tracksPos.size())
        return QString();

    return m_tracksPos.at(position)->filename;
}

int MusicModel::positionFromFilename(const QString &filename) const
{
    if (filename.isEmpty())
        return -1;

    QHash<QString, MusicModelTrack*>::ConstIterator it = m_tracksFile.find(filename);
    if (it == m_tracksFile.end())
        return -1;

    return it.value()->pos;
}

int MusicModel::durationFromFilename(const QString &filename) const
{
    if (filename.isEmpty())
        return -1;

    QHash<QString, MusicModelTrack*>::ConstIterator it = m_tracksFile.find(filename);
    if (it == m_tracksFile.end())
        return 0;

    return it.value()->duration;
}

QString MusicModel::tracknameFromFilename(const QString &filename) const
{
    if (filename.isEmpty())
        return QString();

    QHash<QString, MusicModelTrack*>::ConstIterator it = m_tracksFile.find(filename);
    if (it == m_tracksFile.end())
        return QString();

    return it.value()->track;
}

int MusicModel::trackCount() const
{
    return m_tracksPos.size();
}
