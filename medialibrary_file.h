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
class IOPtr;

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

    QIODevice* deviceForFilename(const QString &filename);

    void setSettings(QSettings *settings);

    void setTag(const QString& filename, const Tag& tag);

    QByteArray mimeType(const QString& filename) const;

signals:
    void tagWritten(const QString& filename);

    void updateStarted();
    void updateFinished();

private slots:
    void jobCreated(IOJob* job);
    void jobFinished(IOJob* job);
    void tagReceived(const Tag& tag);

private:
    void processArtwork(const Tag& tag);
    void syncSettings();

private:
    MediaLibraryFile(QObject *parent = 0);

    QStringList m_paths;
    PathSet m_updatedPaths;

    QSet<QString> m_pendingArtwork;
    QSet<int> m_pendingJobs;
    QHash<IOJob*, IOPtr> m_jobs;
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
