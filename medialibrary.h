#ifndef MEDIALIBRARY_H
#define MEDIALIBRARY_H

#include <QObject>
#include <QStringList>
#include <QSet>
#include <QHash>
#include <QImage>

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
};

class MediaLibrary : public QObject
{
    Q_OBJECT
public:
    static MediaLibrary* instance();
    static void init(QObject* parent = 0);

    ~MediaLibrary();

    QStringList paths();
    void setPaths(const QStringList& paths);
    void addPath(const QString& path);
    void incrementalUpdate();
    void fullUpdate();

    void readLibrary();

    void requestArtwork(const QString& filename);
    void requestTag(const QString& filename);
    void setTag(const QString& filename, const Tag& tag);

signals:
    void artwork(const QImage& image);
    void tag(const Tag& tag);
    void tagWritten(const QString& filename);

    void artist(const Artist& artist);
    void trackRemoved(int trackid);
    void updateStarted();
    void updateFinished();

private slots:
    void jobCreated(IOJob* job);
    void jobFinished(IOJob* job);
    void tagReceived(const Tag& tag);

private:
    void processArtwork(const Tag& tag);

private:
    MediaLibrary(QObject *parent = 0);

    static MediaLibrary* s_inst;

    QStringList m_paths;
    PathSet m_updatedPaths;

    QSet<QString> m_pendingArtwork;
    QSet<int> m_pendingJobs;
    QHash<IOJob*, IOPtr> m_jobs;
};

#endif // MEDIALIBRARY_H
