#include "musicmodel.h"
#include <QDebug>

struct MusicModelArtist
{
    int id;
    QString artist;

    QHash<int, MusicModelAlbum*> albums;
    QHash<int, MusicModelTrack*> tracks;
};

struct MusicModelAlbum
{
    int id;
    QString album;

    MusicModelArtist* artist;
    QHash<int, MusicModelTrack*> tracks;
};

struct MusicModelTrack
{
    int pos;

    int id;
    QString track;
    QString filename;
    int trackno;

    MusicModelArtist* artist;
    MusicModelAlbum* album;
};

static bool trackLessThan(const MusicModelTrack* t1, const MusicModelTrack* t2)
{
    return t1->trackno < t2->trackno;
}

MusicModel::MusicModel(QObject *parent)
    : QAbstractTableModel(parent), m_artist(0), m_album(0)
{
    connect(MediaLibrary::instance(), SIGNAL(artist(Artist)), this, SLOT(updateArtist(Artist)));
    connect(MediaLibrary::instance(), SIGNAL(trackRemoved(int)), this, SLOT(removeTrack(int)));
    MediaLibrary::instance()->readLibrary();

    QHash<int, QByteArray> roles;
    roles[Qt::UserRole + 1] = "musicitem";
    roles[Qt::UserRole + 2] = "musicid";
    roles[Qt::UserRole + 3] = "musicindex";
    setRoleNames(roles);
}

MusicModel::~MusicModel()
{
    qDeleteAll(m_artists);
    qDeleteAll(m_albums);
    qDeleteAll(m_tracks);
}

void MusicModel::updateArtist(const Artist &artist)
{
    bool insertArtist = (!m_artist && !m_album);
    bool insertAlbum = (m_artist && !m_album);
    bool insertTrack = (m_artist && m_album);

    if (insertArtist)
        beginInsertRows(QModelIndex(), m_artists.size(), m_artists.size());

    MusicModelArtist* currentArtist;
    if (!m_artists.contains(artist.id)) {
        currentArtist = new MusicModelArtist;
        m_artists[artist.id] = currentArtist;
    } else
        currentArtist = m_artists[artist.id];

    currentArtist->id = artist.id;
    currentArtist->artist = artist.name;

    if (insertArtist)
        endInsertRows();

    MusicModelAlbum* currentAlbum;
    MusicModelTrack* currentTrack;

    if (insertAlbum && m_artist->id == currentArtist->id && !artist.albums.isEmpty())
        beginInsertRows(QModelIndex(), currentArtist->albums.size(), currentArtist->albums.size() + artist.albums.size() - 1);

    foreach(const Album& album, artist.albums) {
        if (!currentArtist->albums.contains(album.id)) {
            currentAlbum = new MusicModelAlbum;
            currentArtist->albums[album.id] = currentAlbum;
            // Assume the album is not in m_albums either
            m_albums[album.id] = currentAlbum;
        } else
            currentAlbum = currentArtist->albums[album.id];

        currentAlbum->id = album.id;
        currentAlbum->album = album.name;
        currentAlbum->artist = currentArtist;

        if (insertTrack && m_album->id == currentAlbum->id && !album.tracks.isEmpty())
            beginInsertRows(QModelIndex(), currentAlbum->tracks.size(), currentAlbum->tracks.size() + album.tracks.size() - 1);

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
            currentTrack->artist = currentArtist;
            currentTrack->album = currentAlbum;
        }

        if (insertTrack && m_album->id == currentAlbum->id && !album.tracks.isEmpty()) {
            endInsertRows();
            buildTracks();
        }
    }

    if (insertAlbum && m_artist->id == currentArtist->id && !artist.albums.isEmpty())
        endInsertRows();
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
    delete album;
    delete artist;
}

int MusicModel::currentArtist() const
{
    if (m_artist)
        return m_artist->id;
    return 0;
}

void MusicModel::setCurrentArtist(int artist)
{
    MusicModelArtist* oldartist = m_artist;
    MusicModelAlbum* oldalbum = m_album;

    m_tracksPos.clear();
    m_tracksFile.clear();

    if (artist > 0) {
        if (m_artists.contains(artist))
            m_artist = m_artists[artist];
        else
            m_artist = 0;
    } else
        m_artist = 0;
    m_album = 0;

    if (m_artist != oldartist || m_album != oldalbum)
        reset();
}

int MusicModel::currentAlbum() const
{
    if (m_album)
        return m_album->id;
    return 0;
}

void MusicModel::setCurrentAlbum(int album)
{
    MusicModelAlbum* oldalbum = m_album;

    if (m_artist == 0)
        return;

    m_tracksPos.clear();
    m_tracksFile.clear();

    if (album > 0) {
        if (m_artist->albums.contains(album)) {
            m_album = m_artist->albums[album];

            buildTracks();
        } else
            m_album = 0;
    } else
        m_album = 0;

    if (m_album != oldalbum)
        reset();
}

void MusicModel::buildTracks()
{
    if (!m_artist || !m_album)
        return;

    QHash<int, MusicModelTrack*>::ConstIterator it = m_album->tracks.begin();
    QHash<int, MusicModelTrack*>::ConstIterator itend = m_album->tracks.end();
    while (it != itend) {
        m_tracksPos.append(*it);
        m_tracksFile[(*it)->filename] = *it;

        ++it;
    }

    // Sort by track number
    qSort(m_tracksPos.begin(), m_tracksPos.end(), trackLessThan);

    // Initialize the pos(ition) variable
    int pos = 0;
    foreach(MusicModelTrack* track, m_tracksPos) {
        track->pos = pos++;
    }
}

QVariant MusicModel::musicData(const QModelIndex &index, int role) const
{
    QVariant ret;

    // ### This is not really optimal.
    // ### Better to use keep both a hashmap and a list and keep them synchronized probably

    if (role == Qt::DisplayRole) {
        if (!m_artist) {
            QList<MusicModelArtist*> artists = m_artists.values();

            if (index.row() < artists.size()) {
                if (index.column() == 0)
                    ret = artists.at(index.row())->artist;
                else if (index.column() == 1)
                    ret = artists.at(index.row())->id;
            }
            return ret;
        } else if (!m_album) {
            QList<MusicModelAlbum*> albums = m_artist->albums.values();

            if (index.row() < albums.size()) {
                if (index.column() == 0)
                    ret = albums.at(index.row())->album;
                else if (index.column() == 1)
                    ret = albums.at(index.row())->id;
            }
            return ret;
        } else {
            if (index.row() < m_tracksPos.size()) {
                if (index.column() == 0)
                    ret = m_tracksPos.at(index.row())->track;
                else if (index.column() == 1)
                    ret = m_tracksPos.at(index.row())->id;
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
    if (!m_artist)
        return m_artists.size();
    else if (!m_album)
        return m_artist->albums.size();
    return m_tracksPos.size();
}

QVariant MusicModel::data(const QModelIndex &index, int role) const
{
    if (role < Qt::UserRole)
        return musicData(index, role);

    if (role == Qt::UserRole + 3)
        return index.row();

    int col = role - Qt::UserRole - 1;
    QModelIndex idx = this->index(index.row(), col);

    return musicData(idx, Qt::DisplayRole);
}

QVariant MusicModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QAbstractTableModel::headerData(section, orientation, role);
}

QString MusicModel::filenameById(int id) const
{
    if (m_artist == 0 || m_album == 0)
        return QString();

    return m_tracks.value(id)->filename;
}

QString MusicModel::filenameByPosition(int position) const
{
    if (m_artist == 0 || m_album == 0)
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

int MusicModel::trackCount() const
{
    return m_tracksPos.size();
}
