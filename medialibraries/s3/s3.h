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

#ifndef S3_MEDIA_H
#define S3_MEDIA_H

#include "medialibrary.h"
#include "mediareader.h"

class MediaReaderS3Private;
class MediaLibraryS3Private;

class MediaReaderS3 : public MediaReaderInterface
{
public:
    MediaReaderS3(const QString& filename);
    ~MediaReaderS3();

    bool open();

    bool atEnd() const;
    QByteArray readData(qint64 length);

    void pause();
    void resume(qint64 position);

    void setTargetThread(QThread *thread);
    void setDataCallback(MediaReader *reader, MediaReaderCallback callback);

private:
    MediaReaderS3Private* m_priv;
};

class MediaLibraryS3 : public QObject, public MediaLibraryInterface
{
    Q_OBJECT
    Q_INTERFACES(MediaLibraryInterface)
public:
    MediaLibraryS3(QObject* parent = 0);
    ~MediaLibraryS3();

    QString name();

    bool readFirstArtist(Artist* artist);
    bool readNextArtist(Artist* artist);
    void readArtworkForTrack(const QString& filename, QImage* image);
    void readMetaDataForTrack(const QString& filename, Tag* tag);
    MediaReaderInterface* mediaReaderForTrack(const QString& filename);
    QByteArray mimeTypeForTrack(const QString& filename);

private:
    void readS3();

private:
    MediaLibraryS3Private* m_priv;
};

#endif
