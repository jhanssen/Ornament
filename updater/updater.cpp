#include "updater.h"
#include "trackduration.h"
#include "tag.h"
#include "awsconfig.h"
#include "libs3.h"
#include <QTimer>
#include <QApplication>
#include <QTemporaryFile>
#include <QDir>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QUrl>
#include <QDebug>

class Progress : public QWidget
{
public:
    Progress(QWidget* parent = 0);

    QLabel* fileNameLabel;
    QProgressBar* fileProgress;
    QProgressBar* totalProgress;
};

Progress::Progress(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout* vbox = new QVBoxLayout(this);
    QHBoxLayout* hbox = new QHBoxLayout;
    QLabel* totalLabel = new QLabel("Total");
    hbox->addWidget(totalLabel);
    totalProgress = new QProgressBar;
    hbox->addWidget(totalProgress);
    vbox->addLayout(hbox);
    hbox = new QHBoxLayout;
    fileNameLabel = new QLabel;
    hbox->addWidget(fileNameLabel);
    fileProgress = new QProgressBar;
    hbox->addWidget(fileProgress);
    vbox->addLayout(hbox);
}

static QByteArray mimeType(const QString &filename)
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

static int dataCallback(int bufferSize, char* buffer, void* callbackData)
{
    Updater* updater = (Updater*)callbackData;
    int read = updater->readFromCurrent(bufferSize, buffer);
    updater->updateProgress(read);

    //qDebug() << "read" << updater->currentPosition() << "of" << updater->currentSize() << "from" << updater->currentFilename();

    return read;
}

static void completeCallback(S3Status status, const S3ErrorDetails* errorDetails, void* callbackData)
{
    Q_UNUSED(callbackData)

    qDebug() << "complete" << status;

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

Updater::Updater(QObject *parent)
    : QObject(parent), m_totalsize(0), m_writingArtwork(false), m_current(0), m_progress(new Progress)
{
    m_progress->show();
}

Updater::~Updater()
{
    delete m_progress;
}

void Updater::updateProgress(int read)
{
    if (!m_writingArtwork) {
        static int total = 0;
        total += read;
        m_progress->totalProgress->setValue(total);
    }
    m_progress->fileProgress->setValue(m_current->pos());
    QApplication::processEvents();
}

void Updater::updateProgressName(const QString &artist, const QString &album, const QString &track)
{
    m_progress->fileProgress->setMaximum(m_current->size());
    m_progress->fileProgress->setValue(0);

    m_progress->fileNameLabel->setText(track);
    m_progress->topLevelWidget()->setWindowTitle(artist + " / " + album);
}

void Updater::update(const QString &path)
{
    m_path = path;
    QTimer::singleShot(0, this, SLOT(startUpdate()));
}

static inline QByteArray encodeFilename(int trackno, const QString& track, int duration, const QString& ext)
{
    QByteArray t = QUrl::toPercentEncoding(track);
    return QByteArray::number(trackno) + '_' + t + '_' + QByteArray::number(duration) + '.' + ext.toLatin1();
}

void Updater::startUpdate()
{
    updateDirectory(m_path);

    m_progress->totalProgress->setMaximum(m_totalsize);

    S3BucketContext* context = (S3BucketContext*)malloc(sizeof(S3BucketContext));
    context->accessKeyId = AwsConfig::accessKey();
    context->secretAccessKey = AwsConfig::secretKey();
    context->bucketName = AwsConfig::bucket();
    context->protocol = S3ProtocolHTTPS;
    context->uriStyle = S3UriStyleVirtualHost;

    int trackno, duration;
    QString album, artist, track;
    QImage artwork;
    QSet<QString> artworkWritten;

    QByteArray mime;
    foreach(const QFileInfo& info, m_update) {
        mime = mimeType(info.absoluteFilePath());
        if (mime.startsWith("audio/")) {
            Tag tag(info.absoluteFilePath());
            artist = tag.data("artist").toString();
            album = tag.data("album").toString();
            track = tag.data("title").toString();
            trackno = tag.data("track").toInt();
            artwork = tag.data("picture0").value<QImage>();
            duration = 0;
            if (mime == "audio/mp3")
                duration = TrackDuration::duration(info);

            //qDebug() << "updating" << info.absoluteFilePath() << artist << album << track << trackno << artwork.size() << duration;
            m_current = new QFile;
            m_current->setFileName(info.absoluteFilePath());
            if (m_current->open(QFile::ReadOnly)) {
                updateProgressName(artist, album, track);

                QByteArray key = QUrl::toPercentEncoding(artist) + "/" + QUrl::toPercentEncoding(album) + "/" + encodeFilename(trackno, track, duration, info.suffix());
                S3PutObjectHandler objectHandler;
                objectHandler.responseHandler.completeCallback = completeCallback;
                objectHandler.responseHandler.propertiesCallback = propertiesCallback;
                objectHandler.putObjectDataCallback = dataCallback;
                S3_put_object(context, key.constData(), m_current->size(), 0, 0, &objectHandler, this);
            }
            delete m_current;

            if (!artworkWritten.contains(artist + "/" + album) && !artwork.isNull()) {
                artworkWritten.insert(artist + "/" + album);
                m_current = new QTemporaryFile("playerartwork");
                if (static_cast<QTemporaryFile*>(m_current)->open()) {
                    m_writingArtwork = true;
                    QString fn = m_current->fileName();
                    int lastSlash = fn.lastIndexOf(QLatin1Char('/'));
                    if (lastSlash != -1)
                        fn = fn.mid(lastSlash + 1);
                    updateProgressName(artist, album, fn);

                    artwork.save(m_current, "PNG");
                    m_current->seek(0);

                    S3PutObjectHandler objectHandler;
                    objectHandler.responseHandler.completeCallback = completeCallback;
                    objectHandler.responseHandler.propertiesCallback = propertiesCallback;
                    objectHandler.putObjectDataCallback = dataCallback;
                    QByteArray key = QUrl::toPercentEncoding(artist) + "/" + QUrl::toPercentEncoding(album) + "/Folder.png";
                    S3_put_object(context, key.constData(), m_current->size(), 0, 0, &objectHandler, this);
                    m_writingArtwork = false;
                }
                delete m_current;
            }
        }
    }

    free(context);

    qApp->quit();
}

int Updater::readFromCurrent(int bufferSize, char *buffer)
{
    return m_current->read(buffer, bufferSize);
}

QString Updater::currentFilename() const
{
    return m_current->fileName();
}

qint64 Updater::currentPosition() const
{
    return m_current->pos();
}

qint64 Updater::currentSize() const
{
    return m_current->size();
}

void Updater::updateDirectory(const QString &path)
{
    QDir dir(path);

    QList<QFileInfo> list = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable);
    foreach(const QFileInfo& info, list) {
        if (info.isDir())
            updateDirectory(info.absoluteFilePath());
        else if (info.isFile()) {
            if (!mimeType(info.absoluteFilePath()).isEmpty()) {
                m_totalsize += info.size();
                m_update.append(info);
            }
        }
    }
}
