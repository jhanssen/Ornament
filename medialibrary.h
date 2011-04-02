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
#include <QString>
#include <QHash>
#include <QImage>
#include "tag.h"

class AudioReader;
class QSettings;

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

class MediaLibrary : public QObject
{
    Q_OBJECT
public:
    static MediaLibrary* instance();

    virtual void readLibrary() = 0;

    virtual void requestArtwork(const QString& filename) = 0;
    virtual void requestMetaData(const QString& filename) = 0;

    virtual AudioReader* readerForFilename(const QString& filename) = 0;
    virtual QByteArray mimeType(const QString& filename) const = 0;

    virtual void setSettings(QSettings* settings);

signals:
    void artist(const Artist& artist);
    void artwork(const QImage& image);
    void metaData(const Tag& tag);

    void trackRemoved(int trackid);
    void cleared();

protected:
    MediaLibrary(QObject *parent = 0);

    static MediaLibrary* s_inst;

    QSettings* m_settings;
};

#endif // MEDIALIBRARY_H
