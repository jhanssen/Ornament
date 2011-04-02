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

#ifndef MEDIALIBRARY_S3_H
#define MEDIALIBRARY_S3_H

#include "medialibrary.h"

class MediaLibraryS3Private;

class MediaLibraryS3 : public MediaLibrary
{
    Q_OBJECT
public:
    static void init(QObject* parent = 0);

    ~MediaLibraryS3();

    void readLibrary();

    void requestArtwork(const QString& filename);
    void requestMetaData(const QString& filename);

    AudioReader* readerForFilename(const QString &filename);
    QByteArray mimeType(const QString& filename) const;

    void setSettings(QSettings *settings);

private slots:
    void S3complete();

private:
    void readS3();

private:
    friend class MediaLibraryS3Private;

    MediaLibraryS3(QObject* parent = 0);

    MediaLibraryS3Private* priv;
};

#endif // MEDIALIBRARY_S3_H
