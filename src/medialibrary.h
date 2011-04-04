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

#ifndef MEDIALIBRARY_H
#define MEDIALIBRARY_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QImage>
#include <QtPlugin>
#include "tag.h"

class MediaReader;
class MediaReaderInterface;
class MediaLibraryPrivate;

struct Album;
struct Track;

struct Artist
{
    int id;
    QString name;

    QHash<int, Album> albums;
};

struct Album
{
    int id;
    QString name;

    QHash<int, Track> tracks;
};

struct Track
{
    int id;
    QString name;
    QString filename;
    int trackno;
    int duration;
};

class MediaLibraryInterface
{
public:
    virtual ~MediaLibraryInterface() {}

    // These are executed in the IO thread
    virtual bool readFirstArtist(Artist* artist) = 0;
    virtual bool readNextArtist(Artist* artist) = 0;
    virtual void readArtworkForTrack(const QString& filename, QImage* image) = 0;
    virtual void readMetaDataForTrack(const QString& filename, Tag* tag) = 0;

    // These are executed in the thread that MediaLibrary lives in
    virtual MediaReaderInterface* mediaReaderForTrack(const QString& filename) = 0;
    virtual QByteArray mimeTypeForTrack(const QString& filename) = 0;
};

Q_DECLARE_INTERFACE(MediaLibraryInterface, "Ornament.MediaLibraryInterface")

class MediaLibrary : public QObject
{
    Q_OBJECT
public:
    static void init(QObject* parent = 0);
    static MediaLibrary* instance();

    ~MediaLibrary();

    void readLibrary();

    void requestArtwork(const QString& filename);
    void requestMetaData(const QString& filename);

    MediaReader* readerForFilename(const QString &filename);
    QByteArray mimeType(const QString& filename) const;

signals:
    void artist(const Artist& artist);
    void artwork(const QImage& image);
    void metaData(const Tag& tag);
    void trackRemoved(int trackid);
    void cleared();

private:
    friend class MediaLibraryPrivate;

    MediaLibrary(QObject* parent = 0);

    MediaLibraryPrivate* m_priv;
    static MediaLibrary* s_inst;
};

#endif // MEDIALIBRARY_S3_H
