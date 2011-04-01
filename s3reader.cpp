#include "s3reader.h"
#include "io.h"
#include "buffer.h"
#include "awsconfig.h"
#include <libs3.h>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslError>
#include <QTimer>
#include <QDebug>

class S3ReaderJob : public IOJob
{
    Q_OBJECT
public:
    S3ReaderJob(QObject* parent = 0);
    ~S3ReaderJob();

    void start();

    QString filename;

private slots:
    void replyFinished();
    void replyData();
    void replyError(QNetworkReply::NetworkError errorType);
    void replySslErrors(const QList<QSslError>& errors);

private:
    Q_INVOKABLE void startJob();
    S3BucketContext* context;
    QNetworkAccessManager* manager;
    QNetworkReply* reply;

signals:
    void data(QByteArray* data);
    void atEnd();
};

#include "s3reader.moc"

S3ReaderJob::S3ReaderJob(QObject *parent)
    : IOJob(parent), manager(0), reply(0)
{
    context = (S3BucketContext*)malloc(sizeof(S3BucketContext));
    context->accessKeyId = AwsConfig::accessKey();
    context->secretAccessKey = AwsConfig::secretKey();
    context->bucketName = AwsConfig::bucket();
    context->protocol = S3ProtocolHTTPS;
    context->uriStyle = S3UriStyleVirtualHost;
}

S3ReaderJob::~S3ReaderJob()
{
    free(context);
}

void S3ReaderJob::start()
{
    QMetaObject::invokeMethod(this, "startJob");
}

void S3ReaderJob::startJob()
{
    if (!manager) {
        manager = new QNetworkAccessManager(this);
        connect(manager, SIGNAL(destroyed()), this, SIGNAL(finished()));
    }

    char query[S3_MAX_AUTHENTICATED_QUERY_STRING_SIZE];

    QByteArray key = QUrl::toPercentEncoding(filename, "/_");
    S3Status status = S3_generate_authenticated_query_string(query, context, key.constData(), -1, 0);
    if (status != S3StatusOK) {
        qDebug() << "error when generating query string for" << key << "," << status;
        emit finished();
        return;
    }

    QUrl url;
    url.setEncodedUrl(query, QUrl::TolerantMode);

    QNetworkRequest req(url);
    reply = manager->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(replyFinished()));
    connect(reply, SIGNAL(readyRead()), this, SLOT(replyData()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(replyError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(replySslErrors(QList<QSslError>)));
}

void S3ReaderJob::replyFinished()
{
    emit atEnd();
    reply->deleteLater();
    manager->deleteLater();
}

void S3ReaderJob::replyData()
{
    QByteArray* d = new QByteArray(reply->readAll());
    emit data(d);
}

void S3ReaderJob::replyError(QNetworkReply::NetworkError errorType)
{
    qDebug() << "reply error" << errorType;
}

void S3ReaderJob::replySslErrors(const QList<QSslError>& errors)
{
    foreach(const QSslError& error, errors) {
        qDebug() << "reply ssl error" << error.errorString();
    }
}

S3Reader::S3Reader(QObject *parent)
    : QIODevice(parent), m_jobid(0)
{
    connect(IO::instance(), SIGNAL(error(QString)), this, SLOT(ioError(QString)));
    connect(IO::instance(), SIGNAL(jobCreated(IOJob*)), this, SLOT(jobCreated(IOJob*)));
    connect(IO::instance(), SIGNAL(jobFinished(IOJob*)), this, SLOT(jobFinished(IOJob*)));
}

S3Reader::~S3Reader()
{
}

void S3Reader::setFilename(const QString &filename)
{
    m_filename = filename;
}

bool S3Reader::isSequential() const
{
    return true;
}

qint64 S3Reader::bytesAvailable() const
{
    return m_buffer.size();
}

bool S3Reader::atEnd() const
{
    return (m_atend && m_buffer.isEmpty());
}

void S3Reader::close()
{
    if (!isOpen())
        return;

    QIODevice::close();

    m_buffer.clear();
    if (m_reader) {
        m_reader->stop();
        m_jobid = 0;
        m_reader.clear();
    }
}

bool S3Reader::open(OpenMode mode)
{
    if (isOpen())
        close();

    m_atend = false;

    S3ReaderJob* job = new S3ReaderJob;
    job->filename = m_filename;
    m_jobid = IO::instance()->startJob(job);

    return QIODevice::open(mode);
}

qint64 S3Reader::readData(char *data, qint64 maxlen)
{
    if (m_atend && m_buffer.isEmpty()) {
        close();
        return 0;
    }

    QByteArray dt = m_buffer.read(maxlen);

    if (!dt.isEmpty())
        memcpy(data, dt.constData(), dt.size());

    return dt.size();
}

qint64 S3Reader::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)

    return 0;
}

void S3Reader::jobCreated(IOJob *job)
{
    if (!m_reader && job->jobNumber() == m_jobid) {
        m_jobid = 0;

        m_reader = job;

        connect(*m_reader, SIGNAL(data(QByteArray*)), this, SLOT(readerData(QByteArray*)));
        connect(*m_reader, SIGNAL(atEnd()), this, SLOT(readerAtEnd()));

        m_reader.as<S3ReaderJob>()->start();
    }
}

void S3Reader::jobFinished(IOJob *job)
{
    if ((!m_reader && !m_jobid) || m_reader == job) {
        m_jobid = 0;

        m_reader.clear();
    }

    IOJob::deleteIfNeeded(job);
}

void S3Reader::readerData(QByteArray *data)
{
    QObject* from = sender();
    if (from && from != m_reader) {
        delete data;
        return;
    }

    m_buffer.add(data);
}

void S3Reader::readerAtEnd()
{
    QObject* from = sender();
    if (from && from != m_reader)
        return;

    m_atend = true;
}

void S3Reader::ioError(const QString &message)
{
    qDebug() << "s3 reader error" << message;
}
