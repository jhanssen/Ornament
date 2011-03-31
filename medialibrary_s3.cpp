#include "medialibrary_s3.h"
#include "s3reader.h"
#include "awsconfig.h"
#include <libs3.h>
#include <QTimer>
#include <QDebug>

class MediaLibraryS3Private : public QObject
{
    Q_OBJECT
public:
    MediaLibraryS3Private(MediaLibraryS3* parent);
    ~MediaLibraryS3Private();

    void parseContent(const S3ListBucketContent& content);
    void updateMetadata(const char* key, const char* value);

    void emitComplete();

    S3BucketContext* m_context;
    S3ListBucketHandler* m_listHandler;

    QHash<int, Artist> m_artists;

    QHash<QString, int> m_artistIds;
    QHash<QString, int> m_albumIds;
    QHash<QString, int> m_trackIds;

    QHash<int, QString> m_albumart;

    QByteArray m_processing;
    QByteArray m_nextmarker;
    bool m_clearmarker;

    enum Mode { None, List, Tracks } m_mode;

    int m_idcount;

    MediaLibraryS3* q;

public slots:
    void processTracks();

signals:
    void complete();
};

#include "medialibrary_s3.moc"

static S3Status listBucketCallback(int isTruncated, const char* nextmarker, int contentsCount, const S3ListBucketContent* contents,
                                   int commonPrefixesCount, const char** commonPrefixes, void* callbackData)
{
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
    if (errorDetails) {
        if (errorDetails->message)
            qDebug() << errorDetails->message;
        if (errorDetails->resource)
            qDebug() << errorDetails->resource;
        if (errorDetails->furtherDetails)
            qDebug() << errorDetails->furtherDetails;
    }

    MediaLibraryS3Private* priv = reinterpret_cast<MediaLibraryS3Private*>(callbackData);
    priv->emitComplete();
}

static S3Status propertiesCallback(const S3ResponseProperties* properties, void* callbackData)
{
    MediaLibraryS3Private* priv = reinterpret_cast<MediaLibraryS3Private*>(callbackData);
    //qDebug() << "propertycallback for" << priv->m_processing;
    if (priv->m_processing.isEmpty())
        return S3StatusOK;

    for (int i = 0; i < properties->metaDataCount; ++i)
        priv->updateMetadata(properties->metaData[i].name, properties->metaData[i].value);

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

void MediaLibraryS3Private::emitComplete()
{
    emit complete();
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
    QString artist = QString::fromUtf8(artistData.constData(), artistData.size());

    int artistid;
    if (!m_artistIds.contains(artist)) {
        artistid = m_idcount;
        m_artistIds[artist] = artistid;

        Artist a;
        a.id = artistid;
        a.name = artist;
        m_artists[artistid] = a;

        ++m_idcount;
    } else {
        artistid = m_artistIds.value(artist);
    }
    Artist* a = &m_artists[artistid];

    if (items.size() <= 1)
        return;

    const QByteArray& albumData = items.at(1);
    QString album = QString::fromUtf8(albumData.constData(), albumData.size());

    int albumid;
    if (!m_albumIds.contains(artist + "/" + album)) {
        albumid = m_idcount;
        m_albumIds[artist + "/" + album] = albumid;

        Album al;
        al.id = albumid;
        al.name = album;
        a->albums[albumid] = al;

        ++m_idcount;
    } else {
        albumid = m_albumIds.value(artist + "/" + album);
    }
    Album* al = &a->albums[albumid];

    if (items.size() <= 2)
        return;

    const QByteArray& trackData = items.at(2);
    QString track = QString::fromUtf8(trackData.constData(), trackData.size());

    QByteArray mime = MediaLibrary::instance()->mimeType(track);
    if (mime.startsWith("image/") && !m_albumart.contains(albumid)) {
        //qDebug() << "album art for" << (artist + "/" + album) << "is" << (artist + "/" + album + "/" + track);
        m_albumart[albumid] = artist + "/" + album + "/" + track;
        return;
    } else if (!mime.startsWith("audio/"))
        return;

    int trackid;
    if (!m_trackIds.contains(artist + "/" + album + "/" + track)) {
        trackid = m_idcount;
        m_trackIds[artist + "/" + album + "/" + track] = trackid;

        Track t;
        t.id = trackid;
        t.filename = artist + "/" + album + "/" + track;
        al->tracks[trackid] = t;

        qDebug() << "adding track" << artist << album << track << trackid;

        ++m_idcount;
    }
}

void MediaLibraryS3Private::updateMetadata(const char *key, const char *value)
{
    QList<QByteArray> items = m_processing.split('/');
    if (items.size() < 3)
        return;

    const QByteArray& artistData = items.at(0);
    QString artist = QString::fromUtf8(artistData.constData(), artistData.size());

    if (!m_artistIds.contains(artist))
        return;

    //qDebug() << "found artist" << artist;

    Artist& a = m_artists[m_artistIds.value(artist)];

    const QByteArray& albumData = items.at(1);
    QString album = QString::fromUtf8(albumData.constData(), albumData.size());

    if (!m_albumIds.contains(artist + "/" + album))
        return;

    //qDebug() << "found album" << album;

    Album& al = a.albums[m_albumIds.value(artist + "/" + album)];

    const QByteArray& trackData = items.at(2);
    QString track = QString::fromUtf8(trackData.constData(), trackData.size());

    if (!m_trackIds.contains(artist + "/" + album + "/" + track))
        return;

    qDebug() << "found track" << track << key << value;

    Track& t = al.tracks[m_trackIds.value(artist + "/" + album + "/" + track)];

    if (qstrcmp(key, "track") == 0)
        t.name = QString::fromUtf8(value);
    else if (qstrcmp(key, "duration") == 0) {
        QByteArray tmp(value);
        t.duration = tmp.toInt();
    } else if (qstrcmp(key, "trackno") == 0) {
        QByteArray tmp(value);
        t.trackno = tmp.toInt();
    }
}

void MediaLibraryS3Private::processTracks()
{
    m_mode = MediaLibraryS3Private::Tracks;

    S3ResponseHandler responseHandler;
    responseHandler.completeCallback = completeCallback;
    responseHandler.propertiesCallback = propertiesCallback;

    QHash<QString, int>::const_iterator it = m_trackIds.begin();
    QHash<QString, int>::const_iterator itend = m_trackIds.end();
    while (it != itend) {
        const QString& key = it.key();

        m_processing = key.toUtf8();
        S3_head_object(m_context, m_processing.constData(), 0, &responseHandler, this);
        ++it;
    }

    m_processing.clear();

    m_mode = MediaLibraryS3Private::None;

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

    m_artistIds.clear();
    m_albumIds.clear();
    m_trackIds.clear();
}

MediaLibraryS3::MediaLibraryS3(QObject *parent)
    : MediaLibrary(parent), priv(new MediaLibraryS3Private(this))
{
    connect(priv, SIGNAL(complete()), this, SLOT(S3complete()));
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
