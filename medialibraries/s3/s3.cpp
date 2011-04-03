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

#include "s3.h"
#include "awsconfig.h"
#include <libs3.h>
#include <QUrl>
#include <QStringList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslError>
#include <QDebug>

#define S3_MIN_BUFFER_SIZE (8192 * 10)

class MediaReaderS3Private : public QObject
{
    Q_OBJECT
public:
    MediaReaderS3Private(QObject* parent = 0);
    ~MediaReaderS3Private();

    bool open();

    QString m_filename;
    bool m_finished;
    bool m_aborted;
    int m_position;

    QNetworkAccessManager* m_manager;
    QNetworkReply* m_reply;

    S3BucketContext* m_context;

    MediaReader* m_reader;
    MediaReaderCallback m_callback;

private slots:
    void replyData();
    void replyFinished();
    void replyError(QNetworkReply::NetworkError errorType);
    void replySslErrors(const QList<QSslError>& errors);
};

MediaReaderS3Private::MediaReaderS3Private(QObject *parent)
    : QObject(parent), m_finished(false), m_aborted(false), m_position(0), m_manager(0), m_reply(0), m_reader(0)
{
    m_context = (S3BucketContext*)malloc(sizeof(S3BucketContext));
    m_context->accessKeyId = AwsConfig::accessKey();
    m_context->secretAccessKey = AwsConfig::secretKey();
    m_context->bucketName = AwsConfig::bucket();
    m_context->protocol = S3ProtocolHTTPS;
    m_context->uriStyle = S3UriStyleVirtualHost;
}

MediaReaderS3Private::~MediaReaderS3Private()
{
    free(m_context);
}

bool MediaReaderS3Private::open()
{
    if (!m_manager) {
        m_manager = new QNetworkAccessManager(this);
    }

    char query[S3_MAX_AUTHENTICATED_QUERY_STRING_SIZE];

    QByteArray key = QUrl::toPercentEncoding(m_filename, "/_");
    S3Status status = S3_generate_authenticated_query_string(query, m_context, key.constData(), -1, 0);
    if (status != S3StatusOK) {
        qDebug() << "error when generating query string for" << key << "," << status;
        m_finished = true;
        return false;
    }

    QUrl url;
    url.setEncodedUrl(query, QUrl::TolerantMode);

    QNetworkRequest req(url);
    if (m_position > 0) {
        qDebug() << "s3 resuming at" << m_position;
        req.setRawHeader("Range", "bytes=" + QByteArray::number(m_position) + "-");
    }

    qDebug() << "s3 making request";
    m_reply = m_manager->get(req);
    m_reply->setReadBufferSize(S3_MIN_BUFFER_SIZE);

    m_finished = false;
    m_aborted = false;

    connect(m_reply, SIGNAL(readyRead()), this, SLOT(replyData()));
    connect(m_reply, SIGNAL(finished()), this, SLOT(replyFinished()));
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(replyError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(replySslErrors(QList<QSslError>)));

    return true;
}

void MediaReaderS3Private::replyData()
{
    if (m_reader)
        (m_reader->*m_callback)();
}

void MediaReaderS3Private::replyFinished()
{
    if (!m_aborted && sender() == m_reply)
        m_finished = true;
}

void MediaReaderS3Private::replyError(QNetworkReply::NetworkError errorType)
{
    qDebug() << "s3 reply error" << errorType;
}

void MediaReaderS3Private::replySslErrors(const QList<QSslError>& errors)
{
    foreach(const QSslError& error, errors) {
        qDebug() << "reply ssl error" << error.errorString();
    }
}

MediaReaderS3::MediaReaderS3(const QString& filename)
    : m_priv(new MediaReaderS3Private)
{
    m_priv->m_filename = filename;
}

MediaReaderS3::~MediaReaderS3()
{
    delete m_priv;
}

bool MediaReaderS3::open()
{
    return m_priv->open();
}

bool MediaReaderS3::atEnd() const
{
    return (m_priv->m_finished && (!m_priv->m_reply || !m_priv->m_reply->bytesAvailable()));
}

QByteArray MediaReaderS3::readData(qint64 length)
{
    if (!m_priv->m_reply)
        return QByteArray();

    return m_priv->m_reply->read(length);
}

void MediaReaderS3::pause()
{
    if (!m_priv->m_reply)
        return;
    m_priv->m_aborted = true;
    m_priv->m_reply->abort();
}

void MediaReaderS3::resume(qint64 position)
{
    m_priv->m_position = position;
    m_priv->open();
}

void MediaReaderS3::setTargetThread(QThread *thread)
{
    if (m_priv->thread() == QThread::currentThread() && m_priv->thread() != thread)
        m_priv->moveToThread(thread);
}

void MediaReaderS3::setDataCallback(MediaReader *reader, MediaReaderCallback callback)
{
    m_priv->m_reader = reader;
    m_priv->m_callback = callback;
}

class MediaLibraryS3Private : public QObject
{
    Q_OBJECT
public:
    MediaLibraryS3Private(MediaLibraryS3* parent);
    ~MediaLibraryS3Private();

    void parseContent(const S3ListBucketContent& content);
    QImage requestArtwork(const QString& filename);

    S3BucketContext* m_context;
    S3ListBucketHandler* m_listHandler;

    QHash<int, Artist> m_artists;
    QHash<int, Artist>::const_iterator m_artistsIterator;

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

signals:
    void complete();
    void artwork(const QImage& image);
};

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
    Q_UNUSED(callbackData)

    if (errorDetails) {
        if (errorDetails->message)
            qDebug() << errorDetails->message;
        if (errorDetails->resource)
            qDebug() << errorDetails->resource;
        if (errorDetails->furtherDetails)
            qDebug() << errorDetails->furtherDetails;
    }
}

static S3Status propertiesCallback(const S3ResponseProperties* properties, void* callbackData)
{
    Q_UNUSED(properties)
    Q_UNUSED(callbackData)

    return S3StatusOK;
}

MediaLibraryS3Private::MediaLibraryS3Private(MediaLibraryS3 *parent)
    : QObject(parent), m_context(0), m_listHandler(0), m_clearmarker(false), m_mode(None), m_idcount(1)
{
    q = parent;

    S3_initialize(NULL, S3_INIT_ALL);

    if (!AwsConfig::init())
        return;

    m_context = (S3BucketContext*)malloc(sizeof(S3BucketContext));
    m_context->accessKeyId = AwsConfig::accessKey();
    m_context->secretAccessKey = AwsConfig::secretKey();
    m_context->bucketName = AwsConfig::bucket();
    m_context->protocol = S3ProtocolHTTPS;
    m_context->uriStyle = S3UriStyleVirtualHost;
}

MediaLibraryS3Private::~MediaLibraryS3Private()
{
    if (m_context)
        free(m_context);

    S3_deinitialize();
}

QImage MediaLibraryS3Private::requestArtwork(const QString &filename)
{
    if (!m_context)
        return QImage();

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

    QImage image = QImage::fromData(m_artwork);
    m_artwork.clear();
    return image;
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

    QByteArray mime = q->mimeTypeForTrack(track);
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

#include "s3.moc"

MediaLibraryS3::MediaLibraryS3(QObject* parent)
    : QObject(parent), m_priv(new MediaLibraryS3Private(this))
{
}

MediaLibraryS3::~MediaLibraryS3()
{
}

void MediaLibraryS3::readS3()
{
    if (!m_priv->m_context)
        return;

    m_priv->m_mode = MediaLibraryS3Private::List;

    if (!m_priv->m_listHandler) {
        m_priv->m_listHandler = (S3ListBucketHandler*)malloc(sizeof(S3ListBucketHandler));

        S3ResponseHandler responseHandler;
        responseHandler.completeCallback = completeCallback;
        responseHandler.propertiesCallback = propertiesCallback;

        m_priv->m_listHandler->responseHandler = responseHandler;
        m_priv->m_listHandler->listBucketCallback = listBucketCallback;
    }

    if (m_priv->m_nextmarker.isEmpty()) {
        m_priv->m_mode = MediaLibraryS3Private::List;
        qDebug() << "listing" << m_priv->m_context << m_priv->m_listHandler;
        S3_list_bucket(m_priv->m_context, "", 0, "", 1000, 0, m_priv->m_listHandler, m_priv);
    } else {
        m_priv->m_clearmarker = true;
        S3_list_bucket(m_priv->m_context, "", m_priv->m_nextmarker.constData(), "", 1000, 0, m_priv->m_listHandler, m_priv);
    }

    m_priv->m_mode = MediaLibraryS3Private::None;
}

bool MediaLibraryS3::readFirstArtist(Artist *artist)
{
    if (m_priv->m_artists.isEmpty()) {
        readS3();
        m_priv->m_artistIds.clear();
        m_priv->m_albumIds.clear();
    }

    m_priv->m_artistsIterator = m_priv->m_artists.begin();
    if (m_priv->m_artistsIterator == m_priv->m_artists.end())
        return false;

    *artist = m_priv->m_artistsIterator.value();
    return true;
}

bool MediaLibraryS3::readNextArtist(Artist *artist)
{
    if (m_priv->m_artistsIterator == m_priv->m_artists.end())
        return false;

    ++m_priv->m_artistsIterator;

    if (m_priv->m_artistsIterator == m_priv->m_artists.end())
        return false;

    *artist = m_priv->m_artistsIterator.value();
    return true;
}

void MediaLibraryS3::readArtworkForTrack(const QString &filename, QImage *image)
{
    if (!m_priv->m_trackIds.contains(filename))
        return;
    int albumid = m_priv->m_albumToTrack.value(m_priv->m_trackIds.value(filename));
    QImage img = m_priv->requestArtwork(m_priv->m_albumart.value(albumid));
    *image = img;
}

void MediaLibraryS3::readMetaDataForTrack(const QString &filename, Tag *tag)
{
    Q_UNUSED(filename)
    *tag = Tag();
}

MediaReaderInterface* MediaLibraryS3::mediaReaderForTrack(const QString &filename)
{
    return new MediaReaderS3(filename);
}

QByteArray MediaLibraryS3::mimeTypeForTrack(const QString &filename)
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

Q_EXPORT_PLUGIN2(s3, MediaLibraryS3)
