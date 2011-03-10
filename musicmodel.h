#ifndef MUSICMODEL_H
#define MUSICMODEL_H

#include <QSqlQueryModel>
#include <QSqlDatabase>

class MusicModel : public QSqlQueryModel
{
    Q_OBJECT

    Q_PROPERTY(int currentArtist READ currentArtist WRITE setCurrentArtist)
    Q_PROPERTY(int currentAlbum READ currentAlbum WRITE setCurrentAlbum)
public:
    enum Mode { Normal, Album, Artist };

    MusicModel(QObject *parent = 0);

    int currentArtist() const;
    void setCurrentArtist(int artist);

    int currentAlbum() const;
    void setCurrentAlbum(int album);

    QVariant data(const QModelIndex& index, int role) const;

public slots:
    QString filename(int track);

private:
    void refresh(Mode mode = Normal);

private:
    void toggleNormal();
    void toggleArtist();
    void toggleAlbum();

private:
    int m_artist;
    int m_album;

    QSqlDatabase m_db;
};

#endif // MUSICMODEL_H
