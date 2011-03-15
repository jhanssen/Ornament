#include "musicmodel.h"
#include <QDebug>

MusicModel::MusicModel(QObject *parent)
    : QAbstractTableModel(parent), m_artist(0), m_album(0)
{
    connect(MediaLibrary::instance(), SIGNAL(artist(Artist)), this, SLOT(updateArtist(Artist)));
    MediaLibrary::instance()->readLibrary();

    QHash<int, QByteArray> roles;
    roles[Qt::UserRole + 1] = "musicitem";
    roles[Qt::UserRole + 2] = "musicid";
    setRoleNames(roles);
}

void MusicModel::updateArtist(const Artist &artist)
{
    int cur = m_artists.size();

    if (m_album || m_artist) {
        m_album = 0;
        m_artist = 0;

        reset();
    }

    if (!m_artists.contains(artist.id)) {
        emit beginInsertRows(QModelIndex(), cur, cur);
        m_artists[artist.id] = artist;
        emit endInsertRows();
    } else {
        Artist& artist = m_artists[artist.id];
        // ### double lookup here due to foreach() only accepting const arguments. Fix?
        foreach(const Album& album, artist.albums) {
            if (!artist.albums.contains(album.id)) {
                artist.albums[album.id] = album;
            } else {
                foreach(const Track& track, album.tracks) {
                    artist.albums[album.id].tracks[track.id] = track;
                }
            }
        }
    }
}

int MusicModel::currentArtist() const
{
    if (m_artist)
        return m_artist->id;
    return 0;
}

void MusicModel::setCurrentArtist(int artist)
{
    Artist* oldartist = m_artist;
    Album* oldalbum = m_album;

    if (artist > 0) {
        if (m_artists.contains(artist))
            m_artist = &m_artists[artist];
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
    Album* oldalbum = m_album;

    if (m_artist == 0)
        return;

    if (album > 0) {
        if (m_artist->albums.contains(album))
            m_album = &(m_artist->albums[album]);
        else
            m_artist = 0;
    } else
        m_album = 0;

    if (m_album != oldalbum)
        reset();
}

QVariant MusicModel::musicData(const QModelIndex &index, int role) const
{
    QVariant ret;

    // ### This is not really optimal.
    // ### Better to use keep both a hashmap and a list and keep them synchronized probably

    if (role == Qt::DisplayRole) {
        if (!m_artist) {
            QList<Artist> artists = m_artists.values();

            if (index.row() < artists.size()) {
                if (index.column() == 0)
                    ret = artists.at(index.row()).name;
                else if (index.column() == 1)
                    ret = artists.at(index.row()).id;
            }
            return ret;
        } else if (!m_album) {
            QList<Album> albums = m_artist->albums.values();

            if (index.row() < albums.size()) {
                if (index.column() == 0)
                    ret = albums.at(index.row()).name;
                else if (index.column() == 1)
                    ret = albums.at(index.row()).id;
            }
            return ret;
        } else {
            QList<Track> tracks = m_album->tracks.values();

            if (index.row() < tracks.size()) {
                if (index.column() == 0)
                    ret = tracks.at(index.row()).name;
                else if (index.column() == 1)
                    ret = tracks.at(index.row()).id;
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
    return 2;
}

int MusicModel::rowCount(const QModelIndex &parent) const
{
    if (parent != QModelIndex())
        return 0;
    if (!m_artist)
        return m_artists.size();
    else if (!m_album)
        return m_artist->albums.size();
    return m_album->tracks.size();
}

QVariant MusicModel::data(const QModelIndex &index, int role) const
{
    if (role < Qt::UserRole)
        return musicData(index, role);

    int col = role - Qt::UserRole - 1;
    QModelIndex idx = this->index(index.row(), col);

    return musicData(idx, Qt::DisplayRole);
}

QVariant MusicModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QAbstractTableModel::headerData(section, orientation, role);
}

QString MusicModel::filename(int track)
{
    if (m_artist == 0 || m_album == 0)
        return QString();

    return m_album->tracks.value(track).filename;
}
