#include "musicmodel.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>

// create table artists (id integer primary key autoincrement, artist text not null);
// create table albums (id integer primary key autoincrement, album text not null, artistid integer, foreign key(artistid) references artist(id));
// create table tracks (id integer primary key autoincrement, track text not null, filename text not null, artistid integer, albumid integer, foreign key(artistid) references artist(id), foreign key(albumid) references album(id));

MusicModel::MusicModel(QObject *parent) :
    QSqlQueryModel(parent), m_artist(0), m_album(0)
{
    m_db = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"));
    m_db.setDatabaseName("player.db");
    m_db.open();

    QHash<int, QByteArray> roles;
    roles[Qt::UserRole + 1] = "musicitem";
    roles[Qt::UserRole + 2] = "musicid";
    setRoleNames(roles);

    toggleNormal();
}

int MusicModel::currentArtist() const
{
    return m_artist;
}

void MusicModel::setCurrentArtist(int artist)
{
    m_artist = artist;
    m_album = 0;

    if (m_artist == 0)
        toggleNormal();
    else
        toggleArtist();
}

int MusicModel::currentAlbum() const
{
    return m_album;
}

void MusicModel::setCurrentAlbum(int album)
{
    if (m_artist == 0)
        return;

    m_album = album;

    if (m_album == 0)
        toggleArtist();
    else
        toggleAlbum();
}

void MusicModel::toggleNormal()
{
    refresh(Normal);
}

void MusicModel::toggleArtist()
{
    refresh(Artist);
}

void MusicModel::toggleAlbum()
{
    refresh(Album);
}

QVariant MusicModel::data(const QModelIndex &index, int role) const
{
    if (role < Qt::UserRole)
        return QSqlQueryModel::data(index, role);

    int col = role - Qt::UserRole - 1;
    QModelIndex idx = this->index(index.row(), col);

    return QSqlQueryModel::data(idx, Qt::DisplayRole);
}

void MusicModel::refresh(Mode mode)
{
    QString stmt;

    switch (mode) {
    case Normal:
        stmt = QLatin1String("select artists.artist, artists.id as artist from artists");
        break;
    case Artist:
        stmt = QLatin1String("select albums.album, albums.id from artists, albums where artists.id = ") + QString::number(m_artist) + QLatin1String(" and albums.artistid = artists.id");
        break;
    case Album:
        stmt = QLatin1String("select tracks.track, tracks.id from artists, albums, tracks where artists.id = ") + QString::number(m_artist) + QLatin1String(" and albums.id = ") + QString::number(m_album) + QLatin1String(" and albums.artistid = artists.id and tracks.albumid = albums.id");
        break;
    }

    setQuery(stmt, m_db);
}

QString MusicModel::filename(int track)
{
    if (m_artist == 0 || m_album == 0)
        return QString();

    QString stmt = QLatin1String("select tracks.filename from artists, albums, tracks where artists.id = '") + QString::number(m_artist) + QLatin1String("' and albums.id = '") + QString::number(m_album) + QLatin1String("' and tracks.id = '") + QString::number(track) + QLatin1String("' and albums.artistid = artists.id and tracks.albumid = albums.id");

    QSqlQuery query(m_db);
    if (query.exec(stmt) && query.next()) {
        return query.value(0).toString();
    }

    return QString();
}
