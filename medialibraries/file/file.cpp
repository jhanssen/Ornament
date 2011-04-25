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

#include "file.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>

class MediaReaderFilePrivate : public QObject
{
    Q_OBJECT
public:
    MediaReaderFilePrivate(QObject* parent = 0);

    QFile file;
    MediaReader *reader;
    MediaReaderCallback callback;
};

MediaReaderFilePrivate::MediaReaderFilePrivate(QObject *parent)
    : QObject(parent), reader(0)
{
}

class MediaLibraryFilePrivate : public QObject
{
    Q_OBJECT
public:
    MediaLibraryFilePrivate(MediaLibraryFile* parent = 0);
    ~MediaLibraryFilePrivate();

    void createTables();
    bool readArtist(Artist* artist);

    MediaLibraryFile* q;
    QSqlDatabase* db;
    QSqlQuery* qry;
};

MediaLibraryFilePrivate::MediaLibraryFilePrivate(MediaLibraryFile *parent)
    : QObject(parent), q(parent), db(0), qry(0)
{
}

MediaLibraryFilePrivate::~MediaLibraryFilePrivate()
{
    delete qry;
    delete db;
}

void MediaLibraryFilePrivate::createTables()
{
    QSqlQuery q(*db);
    q.exec(QLatin1String("create table artists (id integer primary key autoincrement, artist text not null)"));
    q.exec(QLatin1String("create table albums (id integer primary key autoincrement, album text not null, artistid integer, foreign key(artistid) references artist(id))"));
    q.exec(QLatin1String("create table tracks (id integer primary key autoincrement, track text not null, filename text not null, trackno integer, duration integer, artistid integer, albumid integer, foreign key(artistid) references artist(id), foreign key(albumid) references album(id))"));
}

bool MediaLibraryFilePrivate::readArtist(Artist* artist)
{
    QSqlQuery albumQuery(*db), trackQuery(*db);

    int artistid = qry->value(0).toInt();

    artist->id = artistid;
    artist->name = qry->value(1).toString();

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

        artist->albums[albumData.id] = albumData;
    }

    return true;
}

#include "file.moc"

MediaReaderFile::MediaReaderFile(const QString &filename)
    : m_priv(new MediaReaderFilePrivate)
{
    m_priv->file.setFileName(filename);
}

MediaReaderFile::~MediaReaderFile()
{
    delete m_priv;
}

bool MediaReaderFile::open()
{
    bool ok = m_priv->file.open(QFile::ReadOnly);
    if (ok && m_priv->reader)
        (m_priv->reader->*m_priv->callback)();
    return ok;
}

bool MediaReaderFile::atEnd() const
{
    if (!m_priv->file.isOpen())
        return false;
    return m_priv->file.atEnd();
}

QByteArray MediaReaderFile::readData(qint64 length)
{
    return m_priv->file.read(length);
}

void MediaReaderFile::pause()
{
    m_priv->file.close();
}

void MediaReaderFile::resume(qint64 position)
{
    if (m_priv->file.open(QFile::ReadOnly))
        m_priv->file.seek(position);
}

void MediaReaderFile::setTargetThread(QThread *thread)
{
    if (m_priv->thread() == QThread::currentThread() && m_priv->thread() != thread)
        m_priv->moveToThread(thread);
}

void MediaReaderFile::setDataCallback(MediaReader *reader, MediaReaderCallback callback)
{
    m_priv->reader = reader;
    m_priv->callback = callback;
}

MediaLibraryFile::MediaLibraryFile(QObject *parent)
    : QObject(parent), m_priv(new MediaLibraryFilePrivate(this))
{
}

MediaLibraryFile::~MediaLibraryFile()
{
}

bool MediaLibraryFile::readFirstArtist(Artist *artist)
{
    if (!m_priv->db) {
        m_priv->db = new QSqlDatabase;
        *m_priv->db = QSqlDatabase::addDatabase("QSQLITE");
        m_priv->db->setDatabaseName("player.db");
        m_priv->db->open();

        m_priv->qry = new QSqlQuery(*m_priv->db);
    }

    m_priv->qry->exec("select artists.id, artists.artist from artists");
    if (m_priv->qry->next())
        return m_priv->readArtist(artist);
    else
        m_priv->qry->finish();

    return false;
}

bool MediaLibraryFile::readNextArtist(Artist *artist)
{
    if (!m_priv->qry || !m_priv->qry->isActive())
        return false;

    if (m_priv->qry->next())
        return m_priv->readArtist(artist);
    else
        m_priv->qry->finish();

    return false;
}

void MediaLibraryFile::readArtworkForTrack(const QString &filename, QImage *image)
{
    Tag tag(filename);

    *image = tag.data("picture0").value<QImage>();
    if (image->isNull()) {
        QFileInfo info(filename);
        if (info.exists()) {
            QDir dir = info.absoluteDir();

            QStringList files = dir.entryList((QStringList() << "*.png" << "*.jpg" << "*.jpeg"), QDir::Files, QDir::Name);
            foreach(const QString& file, files) {
                QImage img(dir.absoluteFilePath(file));
                if (!img.isNull()) {
                    *image = img;
                    return;
                }
            }
        }
    }
}

void MediaLibraryFile::readMetaDataForTrack(const QString &filename, Tag *tag)
{
    *tag = Tag(filename);
}

MediaReaderInterface* MediaLibraryFile::mediaReaderForTrack(const QString &filename)
{
    return new MediaReaderFile(filename);
}

QByteArray MediaLibraryFile::mimeTypeForTrack(const QString &filename)
{
    if (filename.isEmpty())
        return QByteArray();

    int extpos = filename.lastIndexOf(QLatin1Char('.'));
    if (extpos > 0) {
        QString ext = filename.mid(extpos);
        if (ext == QLatin1String(".mp3"))
            return QByteArray("audio/mp3");
        else if (ext == QLatin1String(".flac"))
            return QByteArray("audio/flac");
        else if (ext == QLatin1String(".jpg") || ext == QLatin1String(".jpeg"))
            return QByteArray("image/jpeg");
        else if (ext == QLatin1String(".png"))
            return QByteArray("image/png");
    }

    return QByteArray();
}

QString MediaLibraryFile::name()
{
    return QLatin1String("file");
}

Q_EXPORT_PLUGIN2(file, MediaLibraryFile)
