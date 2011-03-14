#ifndef MEDIALIBRARY_H
#define MEDIALIBRARY_H

#include <QObject>
#include <QStringList>

#include "codecs/tag.h"

struct Album;
struct Track;

struct Artist
{
    int id;
    QString name;

    QList<Album> albums;
};

struct Album
{
    int id;
    QString name;

    QList<Track> tracks;
};

struct Track
{
    int id;
    QString name;
    QString filename;
};

class MediaLibrary : public QObject
{
    Q_OBJECT
public:
    static MediaLibrary* instance();
    static void init(QObject* parent = 0);

    QStringList paths();
    void setPaths(const QStringList& paths);
    void addPath(const QString& path);
    void update();

    void readLibrary();

    void requestTag(const QString& filename);
    void setTag(const QString& filename, const Tag& tag);

signals:
    void updateProgress(int percentage);
    void tag(const Tag& tag);
    void artist(const Artist& artist);

private:
    MediaLibrary(QObject *parent = 0);

    static MediaLibrary* s_inst;

    QStringList m_paths;
};

#endif // MEDIALIBRARY_H
