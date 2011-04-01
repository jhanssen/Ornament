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

#define S3_MIN_BUFFER_SIZE (8192 * 10)
#define S3_READ_SIZE (8192 * 50)

class S3ReaderJob : public IOJob
{
    Q_OBJECT
public:
    S3ReaderJob(QObject* parent = 0);
    ~S3ReaderJob();

    void setFilename(const QString& m_filename);

    void readMore();
    void start();

    void pause();
    void resume();

signals:
    void data(QByteArray* data);
    void atEnd();
    void starving();

private slots:
    void replyFinished();
    void replyData();
    void replyError(QNetworkReply::NetworkError errorType);
    void replySslErrors(const QList<QSslError>& errors);

private:
    enum State { Reading, Paused } m_state;

    Q_INVOKABLE void startJob();
    Q_INVOKABLE void readMoreData();
    Q_INVOKABLE void setState(int state); // ### Should really use State here

private:
    void readData();
    void startStreaming();

private:
    S3BucketContext* m_context;
    QNetworkAccessManager* m_manager;
    QNetworkReply* m_reply;
    QString m_filename;
    bool m_replyFinished;
    int m_toread;
    qint64 m_position;
};

#include "s3reader.moc"

S3ReaderJob::S3ReaderJob(QObject *parent)
    : IOJob(parent), m_state(Reading), m_manager(0), m_reply(0), m_replyFinished(false), m_toread(0), m_position(0)
{
    m_context = (S3BucketContext*)malloc(sizeof(S3BucketContext));
    m_context->accessKeyId = AwsConfig::accessKey();
    m_context->secretAccessKey = AwsConfig::secretKey();
    m_context->bucketName = AwsConfig::bucket();
    m_context->protocol = S3ProtocolHTTPS;
    m_context->uriStyle = S3UriStyleVirtualHost;
}

S3ReaderJob::~S3ReaderJob()
{
    free(m_context);
}

void S3ReaderJob::setFilename(const QString &fn)
{
    m_filename = fn;
}

void S3ReaderJob::start()
{
    QMetaObject::invokeMethod(this, "startJob");
}

void S3ReaderJob::pause()
{
    QMetaObject::invokeMethod(this, "setState", Q_ARG(int, Paused));
}

void S3ReaderJob::resume()
{
    QMetaObject::invokeMethod(this, "setState", Q_ARG(int, Reading));
}

void S3ReaderJob::setState(int state)
{
    State oldstate = m_state;
    m_state = static_cast<State>(state);

    if (m_state == oldstate)
        return;

    qDebug() << "s3 state changed to" << m_state;

    switch(m_state) {
    case Paused:
        m_reply->abort();
        break;
    case Reading:
        m_toread = 0;
        startJob();
        break;
    }
}

void S3ReaderJob::readMore()
{
    QMetaObject::invokeMethod(this, "readMoreData");
}

void S3ReaderJob::startJob()
{
    if (!m_manager) {
        m_manager = new QNetworkAccessManager(this);
        connect(m_manager, SIGNAL(destroyed()), this, SIGNAL(finished()));
    }

    char query[S3_MAX_AUTHENTICATED_QUERY_STRING_SIZE];

    QByteArray key = QUrl::toPercentEncoding(m_filename, "/_");
    S3Status status = S3_generate_authenticated_query_string(query, m_context, key.constData(), -1, 0);
    if (status != S3StatusOK) {
        qDebug() << "error when generating query string for" << key << "," << status;
        emit finished();
        return;
    }

    QUrl url;
    url.setEncodedUrl(query, QUrl::TolerantMode);

    QNetworkRequest req(url);
    if (m_position > 0) {
        qDebug() << "s3 resuming at" << m_position;
        req.setRawHeader("Range", "bytes=" + QByteArray::number(m_position) + "-");
    }

    m_reply = m_manager->get(req);
    m_reply->setReadBufferSize(S3_MIN_BUFFER_SIZE);
    connect(m_reply, SIGNAL(finished()), this, SLOT(replyFinished()));
    connect(m_reply, SIGNAL(readyRead()), this, SLOT(replyData()));
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(replyError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(replySslErrors(QList<QSslError>)));
}

void S3ReaderJob::replyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (m_state == Paused || reply != m_reply) {
        if (reply)
            reply->deleteLater();

        qDebug() << "s3 reply was finished (after pausing)";

        if (m_reply == reply)
            m_reply = 0;
        return;
    }

    m_replyFinished = true;

    if (m_reply->bytesAvailable() == 0 || m_reply->error() != QNetworkReply::NoError) {
        qDebug() << "s3 reader finished";

        m_replyFinished = false;

        emit atEnd();
        m_reply->deleteLater();
        m_manager->deleteLater();
    }
}

void S3ReaderJob::readData()
{
    QByteArray* d = new QByteArray(m_reply->read(m_toread));
    if (d->isEmpty()) {
        if (m_replyFinished && m_reply->bytesAvailable() == 0) {
            qDebug() << "s3 reader finished";

            m_replyFinished = false;

            emit atEnd();
            m_reply->deleteLater();
            m_manager->deleteLater();
        }

        delete d;
        return;
    }

    //qDebug() << "s3 read" << d->size() << "bytes";

    m_toread -= d->size();
    emit data(d);

    m_position += d->size();

    if (m_replyFinished && m_reply->bytesAvailable() == 0) {
        qDebug() << "s3 reader finished";

        m_replyFinished = false;

        emit atEnd();
        m_reply->deleteLater();
        m_manager->deleteLater();
    }
}

void S3ReaderJob::readMoreData()
{
    if (m_state == Paused)
        return;

    m_toread += S3_READ_SIZE;
    readData();
}

void S3ReaderJob::replyData()
{
    if (m_toread == 0) {
        emit starving();
        return;
    }

    readData();
}

void S3ReaderJob::replyError(QNetworkReply::NetworkError errorType)
{
    if (errorType == QNetworkReply::OperationCanceledError) // This is expected when pausing
        return;
    qDebug() << "reply error" << errorType;
}

void S3ReaderJob::replySslErrors(const QList<QSslError>& errors)
{
    foreach(const QSslError& error, errors) {
        qDebug() << "reply ssl error" << error.errorString();
    }
}

S3Reader::S3Reader(QObject *parent)
    : AudioReader(parent), m_jobid(0), m_requestedData(false)
{
    connect(IO::instance(), SIGNAL(error(QString)), this, SLOT(ioError(QString)));
    connect(IO::instance(), SIGNAL(jobCreated(IOJob*)), this, SLOT(jobCreated(IOJob*)));
    connect(IO::instance(), SIGNAL(jobFinished(IOJob*)), this, SLOT(jobFinished(IOJob*)));
}

S3Reader::~S3Reader()
{
}

void S3Reader::pause()
{
    if (m_reader)
        m_reader.as<S3ReaderJob>()->pause();
}

void S3Reader::resume()
{
    if (m_reader) {
        m_reader.as<S3ReaderJob>()->resume();
        m_reader.as<S3ReaderJob>()->readMore();
        m_requestedData = true;
    }
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

    AudioReader::close();

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
    job->setFilename(m_filename);
    m_jobid = IO::instance()->startJob(job);

    return AudioReader::open(mode);
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

    if (!m_requestedData && m_buffer.size() < S3_MIN_BUFFER_SIZE && m_reader) {
        qDebug() << "s3 buffer low, requesting more";
        m_reader.as<S3ReaderJob>()->readMore();
        m_requestedData = true;
    }

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
        connect(*m_reader, SIGNAL(starving()), this, SLOT(readerStarving()));

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

void S3Reader::readerStarving()
{
    if (m_buffer.size() < S3_MIN_BUFFER_SIZE && m_reader) {
        qDebug() << "s3 job starvation, requesting more";
        m_reader.as<S3ReaderJob>()->readMore();
        m_requestedData = true;
    }
}

void S3Reader::readerData(QByteArray *data)
{
    QObject* from = sender();
    if (from && from != m_reader) {
        delete data;
        return;
    }

    m_buffer.add(data);

    if (m_requestedData && (m_buffer.size() * 2) > S3_MIN_BUFFER_SIZE)
        m_requestedData = false;
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
