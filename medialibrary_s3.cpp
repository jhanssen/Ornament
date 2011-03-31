#include "medialibrary_s3.h"
#include "libs3.h"

MediaLibraryS3::MediaLibraryS3(QObject *parent)
    : MediaLibrary(parent)
{
}

MediaLibraryS3::~MediaLibraryS3()
{
}

void MediaLibraryS3::init(QObject *parent)
{
    if (!s_inst)
        s_inst = new MediaLibraryS3(parent);
}

void MediaLibraryS3::readLibrary()
{
}

void MediaLibraryS3::requestArtwork(const QString &filename)
{
}

void MediaLibraryS3::requestMetaData(const QString &filename)
{
}

void MediaLibraryS3::setSettings(QSettings *settings)
{
    MediaLibrary::setSettings(settings);
}

QIODevice* MediaLibraryS3::deviceForFilename(const QString &filename)
{
    return 0;
}

QByteArray MediaLibraryS3::mimeType(const QString &filename) const
{
    return "audio/mp3";
}
