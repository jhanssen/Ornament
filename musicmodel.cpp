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

    emit beginInsertRows(QModelIndex(), cur, cur);
    m_artists.append(artist);
    emit endInsertRows();
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
        foreach(const Artist& a, m_artists) {
            if (a.id == artist)
                m_artist = const_cast<Artist*>(&a);
        }
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
        foreach(const Album& a, m_artist->albums) {
            if (a.id == album)
                m_album = const_cast<Album*>(&a);
        }
    } else
        m_album = 0;

    if (m_album != oldalbum)
        reset();
}

QVariant MusicModel::musicData(const QModelIndex &index, int role) const
{
    QVariant ret;
    if (role == Qt::DisplayRole) {
        if (!m_artist) {
            if (index.row() < m_artists.size()) {
                if (index.column() == 0)
                    ret = m_artists.at(index.row()).name;
                else if (index.column() == 1)
                    ret = m_artists.at(index.row()).id;
            }
            return ret;
        } else if (!m_album) {
            if (index.row() < m_artist->albums.size()) {
                if (index.column() == 0)
                    ret = m_artist->albums.at(index.row()).name;
                else if (index.column() == 1)
                    ret = m_artist->albums.at(index.row()).id;
            }
            return ret;
        } else {
            if (index.row() < m_album->tracks.size()) {
                if (index.column() == 0)
                    ret = m_album->tracks.at(index.row()).name;
                else if (index.column() == 1)
                    ret = m_album->tracks.at(index.row()).id;
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

    foreach(const Track& t, m_album->tracks) {
        if (t.id == track)
            return t.filename;
    }

    return QString();
}
