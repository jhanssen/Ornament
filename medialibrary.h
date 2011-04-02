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
#include <QStringList>
#include <QSet>
#include <QHash>
#include <QImage>
#include <QAbstractListModel>
#include <QSettings>

#include "tag.h"

typedef QSet<QString> PathSet;

struct Album;
struct Track;
class IOJob;
class IOPtr;

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
    static void init(QObject* parent = 0);

    ~MediaLibrary();

    void setSettings(QSettings* settings);

    QStringList paths();
    void setPaths(const QStringList& paths);
    void addPath(const QString& path);
    void incrementalUpdate();
    void fullUpdate();
    void refresh();

    void readLibrary();

    void requestArtwork(const QString& filename);
    void requestTag(const QString& filename);
    void setTag(const QString& filename, const Tag& tag);

    static QByteArray mimeType(const QString& filename);

signals:
    void artwork(const QImage& image);
    void tag(const Tag& tag);
    void tagWritten(const QString& filename);

    void artist(const Artist& artist);
    void trackRemoved(int trackid);
    void updateStarted();
    void updateFinished();
    void cleared();

private slots:
    void jobReady(IOJob* job);
    void jobFinished(IOJob* job);
    void tagReceived(const Tag& tag);

private:
    void processArtwork(const Tag& tag);
    void syncSettings();

private:
    MediaLibrary(QObject *parent = 0);

    static MediaLibrary* s_inst;

    QStringList m_paths;
    PathSet m_updatedPaths;

    QSet<QString> m_pendingArtwork;
    QHash<IOJob*, IOPtr> m_jobs;

    QSettings* m_settings;
};

class MediaModel : public QAbstractListModel
{
    Q_OBJECT
public:
    MediaModel(QObject* parent = 0);

    Qt::ItemFlags flags(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role);
    bool insertRows(int row, int count, const QModelIndex &parent);
    bool removeRows(int row, int count, const QModelIndex &parent);

    Q_INVOKABLE void updateFromLibrary();
    Q_INVOKABLE void addRow();
    Q_INVOKABLE void removeRow(int row);
    Q_INVOKABLE void setPathInRow(int row);
    Q_INVOKABLE void refreshMedia();

private:
    QStringList m_data;
};

#endif // MEDIALIBRARY_H
