#include "s3reader.h"
#include "io.h"
#include "buffer.h"
#include "awsconfig.h"
#include <libs3.h>
#include <QUrl>
#include <QDebug>

class S3ReaderJob : public IOJob
{
    Q_OBJECT
public:
    S3ReaderJob(QObject* parent = 0);
    ~S3ReaderJob();

    void start();

    QString filename;

    void emitAtEnd();
    void emitData(const char* data, int size);

private:
    Q_INVOKABLE void startJob();
    S3BucketContext* context;

signals:
    void data(QByteArray* data);
    void atEnd();
};

#include "s3reader.moc"

static S3Status dataCallback(int bufferSize, const char* buffer, void* callbackData)
{
    S3ReaderJob* job = (S3ReaderJob*)callbackData;
    job->emitData(buffer, bufferSize);

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

    S3ReaderJob* job = (S3ReaderJob*)callbackData;
    job->emitAtEnd();

    qDebug() << "complete!";
}

static S3Status propertiesCallback(const S3ResponseProperties* properties, void* callbackData)
{
    Q_UNUSED(properties)
    Q_UNUSED(callbackData)

    return S3StatusOK;
}

S3ReaderJob::S3ReaderJob(QObject *parent)
    : IOJob(parent)
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
    S3GetObjectHandler objectHandler;
    objectHandler.responseHandler.completeCallback = completeCallback;
    objectHandler.responseHandler.propertiesCallback = propertiesCallback;
    objectHandler.getObjectDataCallback = dataCallback;
    QByteArray key = QUrl::toPercentEncoding(filename, "/_");
    S3_get_object(context, key.constData(), 0, 0, 0, 0, &objectHandler, this);
}

void S3ReaderJob::emitAtEnd()
{
    emit atEnd();
}

void S3ReaderJob::emitData(const char *d, int size)
{
    QByteArray* array = new QByteArray(d, size);
    emit data(array);
}

S3Reader::S3Reader(QObject *parent)
    : QIODevice(parent)
{
    connect(IO::instance(), SIGNAL(error(QString)), this, SLOT(ioError(QString)));
    connect(IO::instance(), SIGNAL(jobReady(IOJob*)), this, SLOT(jobReady(IOJob*)));
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
    IO::instance()->startJob(job);
    m_reader = job;

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

void S3Reader::jobReady(IOJob *job)
{
    if (m_reader == job) {
        connect(*m_reader, SIGNAL(data(QByteArray*)), this, SLOT(readerData(QByteArray*)));
        connect(*m_reader, SIGNAL(atEnd()), this, SLOT(readerAtEnd()));

        m_reader.as<S3ReaderJob>()->start();
    }
}

void S3Reader::jobFinished(IOJob *job)
{
    if (m_reader == job)
        m_reader.clear();

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
