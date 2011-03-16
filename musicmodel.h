#ifndef MUSICMODEL_H
#define MUSICMODEL_H

#include "medialibrary.h"
#include <QAbstractTableModel>
#include <QList>

class MusicModel : public QAbstractTableModel
{
    Q_OBJECT

    Q_PROPERTY(int currentArtist READ currentArtist WRITE setCurrentArtist)
    Q_PROPERTY(int currentAlbum READ currentAlbum WRITE setCurrentAlbum)
public:
    MusicModel(QObject *parent = 0);

    int currentArtist() const;
    void setCurrentArtist(int artist);

    int currentAlbum() const;
    void setCurrentAlbum(int album);

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex& index, int role) const;
    int rowCount(const QModelIndex& parent) const;
    int columnCount(const QModelIndex& parent) const;

    Q_INVOKABLE QString filenameById(int track) const;
    Q_INVOKABLE QString filenameByPosition(int position) const;

private slots:
    void updateArtist(const Artist& artist);

private:
    QVariant musicData(const QModelIndex& index, int role) const;

private:
    Artist* m_artist;
    Album* m_album;

    QHash<int, Artist> m_artists;
};

#endif // MUSICMODEL_H
