#ifndef MEDIALIBRARY_S3_H
#define MEDIALIBRARY_S3_H

#include "medialibrary.h"

class MediaLibraryS3 : public MediaLibrary
{
    Q_OBJECT
public:
    static void init(QObject* parent = 0);

    ~MediaLibraryS3();

    void readLibrary();

    void requestArtwork(const QString& filename);
    void requestMetaData(const QString& filename);

    QIODevice* deviceForFilename(const QString &filename);
    QByteArray mimeType(const QString& filename) const;

    void setSettings(QSettings *settings);

private:
    MediaLibraryS3(QObject* parent = 0);
};

#endif // MEDIALIBRARY_S3_H
