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

#include "medialibrary_s3.h"
#include "s3reader.h"
#include "awsconfig.h"
#include <libs3.h>
#include <QTimer>
#include <QStringList>
#include <QUrl>
#include <QDebug>

class MediaLibraryS3Private : public QObject
{
    Q_OBJECT
public:
    MediaLibraryS3Private(MediaLibraryS3* parent);
    ~MediaLibraryS3Private();

    void parseContent(const S3ListBucketContent& content);
    void requestArtwork(const QString& filename);

    void emitComplete();
    void emitArtwork();

    S3BucketContext* m_context;
    S3ListBucketHandler* m_listHandler;

    QHash<int, Artist> m_artists;

    QHash<QString, int> m_artistIds;
    QHash<QString, int> m_albumIds;
    QHash<QString, int> m_trackIds;

    QHash<int, int> m_albumToTrack;
    QHash<int, QString> m_albumart;

    QByteArray m_nextmarker;
    QByteArray m_artwork;
    bool m_clearmarker;

    enum Mode { None, List, Tracks, Artwork } m_mode;

    int m_idcount;

    MediaLibraryS3* q;

public slots:
    void processTracks();

signals:
    void complete();
    void artwork(const QImage& image);
};

#include "medialibrary_s3.moc"

static S3Status dataCallback(int bufferSize, const char* buffer, void* callbackData)
{
    MediaLibraryS3Private* priv = reinterpret_cast<MediaLibraryS3Private*>(callbackData);
    priv->m_artwork += QByteArray::fromRawData(buffer, bufferSize);

    return S3StatusOK;
}

static S3Status listBucketCallback(int isTruncated, const char* nextmarker, int contentsCount, const S3ListBucketContent* contents,
                                   int commonPrefixesCount, const char** commonPrefixes, void* callbackData)
{
    Q_UNUSED(commonPrefixesCount)
    Q_UNUSED(commonPrefixes)

    MediaLibraryS3Private* priv = reinterpret_cast<MediaLibraryS3Private*>(callbackData);

    for (int i = 0; i < contentsCount; ++i) {
        priv->parseContent(contents[i]);
    }
    /*
    for (int i = 0; i < commonPrefixesCount; ++i) {
        //priv->addPrefix(commonPrefixes[i]);
        qDebug() << "prefix" << commonPrefixes[i];
    }
    */
    if (isTruncated) {
        priv->m_nextmarker = QByteArray(nextmarker);
        priv->m_clearmarker = false;
    }
    return S3StatusOK;
}

static void completeCallback(S3Status status, const S3ErrorDetails* errorDetails, void* callbackData)
{
    Q_UNUSED(status)

    if (errorDetails) {
        if (errorDetails->message)
            qDebug() << errorDetails->message;
        if (errorDetails->resource)
            qDebug() << errorDetails->resource;
        if (errorDetails->furtherDetails)
            qDebug() << errorDetails->furtherDetails;
    }

    MediaLibraryS3Private* priv = reinterpret_cast<MediaLibraryS3Private*>(callbackData);
    if (priv->m_mode == MediaLibraryS3Private::List)
        priv->emitComplete();
    else if (priv->m_mode == MediaLibraryS3Private::Artwork)
        priv->emitArtwork();
}

static S3Status propertiesCallback(const S3ResponseProperties* properties, void* callbackData)
{
    Q_UNUSED(properties)
    Q_UNUSED(callbackData)

    return S3StatusOK;
}

MediaLibraryS3Private::MediaLibraryS3Private(MediaLibraryS3 *parent)
    : QObject(parent), m_listHandler(0), m_clearmarker(false), m_mode(None), m_idcount(1)
{
    q = parent;

    S3_initialize(NULL, S3_INIT_ALL);

    m_context = (S3BucketContext*)malloc(sizeof(S3BucketContext));
    m_context->accessKeyId = AwsConfig::accessKey();
    m_context->secretAccessKey = AwsConfig::secretKey();
    m_context->bucketName = AwsConfig::bucket();
    m_context->protocol = S3ProtocolHTTPS;
    m_context->uriStyle = S3UriStyleVirtualHost;
}

MediaLibraryS3Private::~MediaLibraryS3Private()
{
    free(m_context);

    S3_deinitialize();
}

void MediaLibraryS3Private::requestArtwork(const QString &filename)
{
    m_mode = Artwork;

    // ### this should run in the IO thread though might not be able to since S3Reader might be blocking it
    // ### Perhaps use the non-blocking thingys of libs3 instead
    S3GetObjectHandler objectHandler;
    objectHandler.responseHandler.completeCallback = completeCallback;
    objectHandler.responseHandler.propertiesCallback = propertiesCallback;
    objectHandler.getObjectDataCallback = dataCallback;
    QByteArray key = QUrl::toPercentEncoding(filename, "/_");
    S3_get_object(m_context, key.constData(), 0, 0, 0, 0, &objectHandler, this);

    m_mode = None;
}

void MediaLibraryS3Private::emitComplete()
{
    emit complete();
}

void MediaLibraryS3Private::emitArtwork()
{
    QImage image = QImage::fromData(m_artwork);
    m_artwork.clear();

    if (image.isNull())
        return;

    emit artwork(image);
}

static bool parseTrack(Track* track, const QString& artist, const QString& album, const QString& trackname)
{
    int ext = trackname.lastIndexOf(QLatin1Char('.'));
    if (ext == -1)
        return false;

    QStringList parts = trackname.left(ext).split(QLatin1Char('_'));
    if (parts.size() != 3)
        return false;

    bool ok;
    track->trackno = parts.at(0).toInt(&ok);
    if (!ok)
        return false;
    track->duration = parts.at(2).toInt(&ok);
    if (!ok)
        return false;
    QString name = parts.at(1);
    name.replace(QLatin1Char('~'), QLatin1Char('/'));
    track->name = name;
    track->filename = artist + "/" + album + "/" + trackname;
    return true;
}

void MediaLibraryS3Private::parseContent(const S3ListBucketContent &content)
{
    QByteArray key = QByteArray::fromRawData(content.key, qstrlen(content.key));
    if (key.endsWith('/'))
        key.chop(1);
    QList<QByteArray> items = key.split('/');
    if (items.isEmpty())
        return;

    const QByteArray& artistData = items.at(0);
    QString artist = QUrl::fromPercentEncoding(artistData);
    QString artistlow = artist.toLower();

    int artistid;
    if (!m_artistIds.contains(artistlow)) {
        artistid = m_idcount;
        m_artistIds[artistlow] = artistid;

        Artist a;
        a.id = artistid;
        a.name = artist;
        m_artists[artistid] = a;

        ++m_idcount;
    } else {
        artistid = m_artistIds.value(artistlow);
    }
    Artist* a = &m_artists[artistid];

    if (items.size() <= 1)
        return;

    const QByteArray& albumData = items.at(1);
    QString album = QUrl::fromPercentEncoding(albumData);
    QString albumlow = album.toLower();

    int albumid;
    if (!m_albumIds.contains(artistlow + "/" + albumlow)) {
        albumid = m_idcount;
        m_albumIds[artistlow + "/" + albumlow] = albumid;

        Album al;
        al.id = albumid;
        al.name = album;
        a->albums[albumid] = al;

        ++m_idcount;
    } else {
        albumid = m_albumIds.value(artistlow + "/" + albumlow);
    }
    Album* al = &a->albums[albumid];

    if (items.size() <= 2)
        return;

    const QByteArray& trackData = items.at(2);
    QString track = QUrl::fromPercentEncoding(trackData);

    QByteArray mime = MediaLibrary::instance()->mimeType(track);
    if (mime.startsWith("image/")
        && (!m_albumart.contains(albumid)) || (track.toLower().startsWith("folder"))) {
        //qDebug() << "album art for" << (artist + "/" + album) << "is" << (artist + "/" + album + "/" + track);
        m_albumart[albumid] = artist + "/" + album + "/" + track;
        return;
    } else if (!mime.startsWith("audio/"))
        return;

    int trackid;
    if (!m_trackIds.contains(artist + "/" + album + "/" + track)) {
        Track t;

        if (parseTrack(&t, artist, album, track)) {
            trackid = m_idcount;
            t.id = trackid;
            m_trackIds[artist + "/" + album + "/" + track] = trackid;

            al->tracks[trackid] = t;

            m_albumToTrack[trackid] = al->id;

            ++m_idcount;
        }
    }
}

void MediaLibraryS3Private::processTracks()
{
    foreach(const Artist& a, m_artists) {
        qDebug() << "emitting artist" << a.name;
        foreach(const Album& album, a.albums) {
            qDebug() << "    " << album.name;
            foreach(const Track& track, album.tracks) {
                qDebug() << "        " << track.name << track.filename;
            }
        }

        emit q->artist(a);
    }

    m_mode = MediaLibraryS3Private::None;

    m_artistIds.clear();
    m_albumIds.clear();
}

MediaLibraryS3::MediaLibraryS3(QObject *parent)
    : MediaLibrary(parent), priv(new MediaLibraryS3Private(this))
{
    connect(priv, SIGNAL(complete()), this, SLOT(S3complete()));
    connect(priv, SIGNAL(artwork(QImage)), this, SIGNAL(artwork(QImage)));
}

MediaLibraryS3::~MediaLibraryS3()
{
}

void MediaLibraryS3::init(QObject *parent)
{
    if (!s_inst)
        s_inst = new MediaLibraryS3(parent);
}

void MediaLibraryS3::readS3()
{
    if (!priv->m_listHandler) {
        priv->m_listHandler = (S3ListBucketHandler*)malloc(sizeof(S3ListBucketHandler));

        S3ResponseHandler responseHandler;
        responseHandler.completeCallback = completeCallback;
        responseHandler.propertiesCallback = propertiesCallback;

        priv->m_listHandler->responseHandler = responseHandler;
        priv->m_listHandler->listBucketCallback = listBucketCallback;
    }

    if (priv->m_nextmarker.isEmpty()) {
        priv->m_mode = MediaLibraryS3Private::List;
        S3_list_bucket(priv->m_context, "", 0, "", 1000, 0, priv->m_listHandler, priv);
    } else {
        priv->m_clearmarker = true;
        S3_list_bucket(priv->m_context, "", priv->m_nextmarker.constData(), "", 1000, 0, priv->m_listHandler, priv);
    }
}

void MediaLibraryS3::readLibrary()
{
    priv->m_nextmarker.clear();
    readS3();
}

void MediaLibraryS3::S3complete()
{
    if (priv->m_mode == MediaLibraryS3Private::List) {
        if (priv->m_clearmarker) {
            priv->m_nextmarker.clear();
            priv->m_clearmarker = false;
        }
        if (!priv->m_nextmarker.isEmpty())
            readS3();
        else {
            priv->m_idcount = 1;
            QTimer::singleShot(0, priv, SLOT(processTracks()));
        }
    }
}

void MediaLibraryS3::requestArtwork(const QString &filename)
{
    if (!priv->m_trackIds.contains(filename))
        return;
    int albumid = priv->m_albumToTrack.value(priv->m_trackIds.value(filename));
    priv->requestArtwork(priv->m_albumart.value(albumid));
}

void MediaLibraryS3::requestMetaData(const QString &filename)
{
    Q_UNUSED(filename)
}

void MediaLibraryS3::setSettings(QSettings *settings)
{
    MediaLibrary::setSettings(settings);
}

AudioReader* MediaLibraryS3::readerForFilename(const QString &filename)
{
    S3Reader* s3reader = new S3Reader;
    s3reader->setFilename(filename);
    return s3reader;
}

QByteArray MediaLibraryS3::mimeType(const QString &filename) const
{
    if (filename.isEmpty())
        return QByteArray();

    int extpos = filename.lastIndexOf(QLatin1Char('.'));
    if (extpos > 0) {
        QString ext = filename.mid(extpos);
        if (ext == QLatin1String(".mp3"))
            return QByteArray("audio/mp3");
        else if (ext == QLatin1String(".jpg") || ext == QLatin1String(".jpeg"))
            return QByteArray("image/jpeg");
        else if (ext == QLatin1String(".png"))
            return QByteArray("image/png");
    }

    return QByteArray();
}
