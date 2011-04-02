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

#ifndef MEDIALIBRARY_FILE_H
#define MEDIALIBRARY_FILE_H

#include <QObject>
#include <QStringList>
#include <QSet>
#include <QHash>
#include <QImage>
#include <QAbstractListModel>
#include <QSettings>

#include "medialibrary.h"
#include "tag.h"

typedef QSet<QString> PathSet;

class IOJob;
class MediaJob;

class MediaLibraryFile : public MediaLibrary
{
    Q_OBJECT
public:
    static void init(QObject* parent = 0);

    ~MediaLibraryFile();

    QStringList paths();
    void setPaths(const QStringList& paths);
    void addPath(const QString& path);
    void incrementalUpdate();
    void fullUpdate();
    void refresh();

    void readLibrary();

    void requestArtwork(const QString& filename);
    void requestMetaData(const QString& filename);

    AudioReader* readerForFilename(const QString &filename);

    void setSettings(QSettings *settings);

    void setTag(const QString& filename, const Tag& tag);

    QByteArray mimeType(const QString& filename) const;

signals:
    void tagWritten(const QString& filename);

    void updateStarted();
    void updateFinished();

private slots:
    void jobStarted();
    void jobFinished();
    void tagReceived(const Tag& tag);

private:
    void processArtwork(const Tag& tag);
    void syncSettings();
    void startJob(IOJob* job);

private:
    MediaLibraryFile(QObject *parent = 0);

    QStringList m_paths;
    PathSet m_updatedPaths;

    QSet<QString> m_pendingArtwork;
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
