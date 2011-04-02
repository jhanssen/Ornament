/*
    Ornament - A cross plaform audio player
    Copyright (C) 2011  Jan Erik Hanssen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MUSICMODEL_H
#define MUSICMODEL_H

#include "medialibrary.h"
#include <QAbstractTableModel>
#include <QList>

class MusicModelArtist;
class MusicModelAlbum;
class MusicModelTrack;

class MusicModel : public QAbstractTableModel
{
    Q_OBJECT

    Q_PROPERTY(int currentArtistId READ currentArtistId WRITE setCurrentArtistId)
    Q_PROPERTY(int currentAlbumId READ currentAlbumId WRITE setCurrentAlbumId)
    Q_PROPERTY(MusicModelArtist* currentArtist READ currentArtist)
    Q_PROPERTY(MusicModelAlbum* currentAlbum READ currentAlbum)
public:
    MusicModel(QObject *parent = 0);
    ~MusicModel();

    MusicModelArtist* currentArtist() const;
    int currentArtistId() const;
    void setCurrentArtistId(int artist);

    MusicModelAlbum* currentAlbum() const;
    int currentAlbumId() const;
    void setCurrentAlbumId(int album);

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex& index, int role) const;
    int rowCount(const QModelIndex& parent) const;
    int columnCount(const QModelIndex& parent) const;

    Q_INVOKABLE QString filenameById(int track) const;
    Q_INVOKABLE QString filenameByPosition(int position) const;
    Q_INVOKABLE QString artistnameFromFilename(const QString& filename) const;
    Q_INVOKABLE QString albumnameFromFilename(const QString& filename) const;
    Q_INVOKABLE QString tracknameFromFilename(const QString& filename) const;
    Q_INVOKABLE int positionFromFilename(const QString& filename) const;
    Q_INVOKABLE int trackCount() const;
    Q_INVOKABLE int durationFromFilename(const QString& filename) const;

private slots:
    void updateArtist(const Artist& artist);
    void removeTrack(int trackid);
    void clearData();

private:
    QVariant musicData(const QModelIndex& index, int role, bool hasAllEntry) const;

    void buildTracks();

private:
    MusicModelArtist* m_artist;
    MusicModelAlbum* m_album;
    bool m_artistEmpty;
    bool m_albumEmpty;

    QHash<int, MusicModelArtist*> m_artists;
    QHash<int, MusicModelAlbum*> m_albums;
    QHash<int, MusicModelTrack*> m_tracks;

    QHash<QString, MusicModelTrack*> m_tracksFile;
    QList<MusicModelTrack*> m_tracksPos;
};

#endif // MUSICMODEL_H
