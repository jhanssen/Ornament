#include "filereader.h"
#include <QDebug>

#define FILEREADERDEVICE_READ 8192
#define FILEREADERDEVICE_MIN (8192 * 4)
#define FILEREADERDEVICE_MAX (8192 * 10)

class FileJob : public IOJob
{
    Q_OBJECT

    Q_PROPERTY(QString filename READ filename WRITE setFilename)
public:
    Q_INVOKABLE FileJob(QObject *parent = 0);

    void read(int size);

    QString filename() const;
    void setFilename(const QString& filename);

    void start();

signals:
    void started();
    void data(QByteArray* data);
    void atEnd();

private:
    Q_INVOKABLE void readData(int size);
    Q_INVOKABLE void startJob();

private:
    QString m_filename;
    QFile m_file;
};

#include "filereader.moc"

FileJob::FileJob(QObject *parent)
    : IOJob(parent)
{
}

void FileJob::start()
{
    QMetaObject::invokeMethod(this, "startJob");
}

void FileJob::startJob()
{
    m_file.setFileName(filename());
    if (!m_file.open(QFile::ReadOnly))
        emit error(QLatin1String("Unable to open file: ") + filename());

    emit started();
}

void FileJob::read(int size)
{
    QMetaObject::invokeMethod(this, "readData", Q_ARG(int, size));
}

void FileJob::readData(int size)
{
    if (!m_file.isOpen()) {
        emit error(QLatin1String("File is not open: ") + filename());
        return;
    }

    QByteArray* dt = new QByteArray(m_file.read(size));
    emit data(dt);

    if (m_file.atEnd()) {
        emit atEnd();
        m_file.close();

        emit finished();
    }
}

QString FileJob::filename() const
{
    return m_filename;
}

void FileJob::setFilename(const QString &filename)
{
    m_filename = filename;
}

FileReader::FileReader(QObject *parent)
    : AudioReader(parent), m_jobid(0), m_atend(false), m_pendingTotal(0)
{
    connect(IO::instance(), SIGNAL(jobCreated(IOJob*)), this, SLOT(jobCreated(IOJob*)));
    connect(IO::instance(), SIGNAL(jobFinished(IOJob*)), this, SLOT(jobFinished(IOJob*)));
    connect(IO::instance(), SIGNAL(error(QString)), this, SLOT(ioError(QString)));
}

FileReader::FileReader(const QString &filename, QObject *parent)
    : AudioReader(parent), m_filename(filename), m_jobid(0), m_atend(false), m_pendingTotal(0)
{
    connect(IO::instance(), SIGNAL(jobCreated(IOJob*)), this, SLOT(jobCreated(IOJob*)));
    connect(IO::instance(), SIGNAL(jobFinished(IOJob*)), this, SLOT(jobFinished(IOJob*)));
    connect(IO::instance(), SIGNAL(error(QString)), this, SLOT(ioError(QString)));
}

FileReader::~FileReader()
{
    close();
}

void FileReader::ioError(const QString &message)
{
    qDebug() << "ioerror" << message;
    close();
}

void FileReader::setFilename(const QString &filename)
{
    m_filename = filename;
}

bool FileReader::isSequential() const
{
    return true;
}

qint64 FileReader::bytesAvailable() const
{
    return m_buffer.size();
}

void FileReader::close()
{
    if (!isOpen())
        return;

    AudioReader::close();
    m_atend = false;

    m_buffer.clear();
    if (m_reader) {
        m_reader->stop();
        m_jobid = 0;
        m_reader.clear();
        m_pending.clear();
        m_pendingTotal = 0;
    }
}

bool FileReader::open(OpenMode mode)
{
    if (m_filename.isEmpty())
        return false;
    bool ok = AudioReader::open(mode);
    if (!ok)
        return false;

    m_atend = false;

    m_buffer.clear();
    if (m_reader) {
        m_reader->stop();
        m_jobid = 0;
        m_reader.clear();
        m_pending.clear();
        m_pendingTotal = 0;
    }

    FileJob* job = new FileJob;
    job->setFilename(m_filename);
    m_jobid = IO::instance()->startJob(job);

    return true;
}

void FileReader::jobCreated(IOJob *job)
{
    if (!m_reader && job->jobNumber() == m_jobid) {
        m_jobid = 0;

        m_reader = job;
        /*
        if (!m_reader)
            return;
            */

        connect(*m_reader, SIGNAL(started()), this, SLOT(readerStarted()));
        connect(*m_reader, SIGNAL(data(QByteArray*)), this, SLOT(readerData(QByteArray*)));
        connect(*m_reader, SIGNAL(atEnd()), this, SLOT(readerAtEnd()));
        connect(*m_reader, SIGNAL(error(QString)), this, SLOT(readerError(QString)));

        m_reader.as<FileJob>()->start();
    }
}

void FileReader::jobFinished(IOJob *job)
{
    if ((!m_reader && !m_jobid) || m_reader == job) {
        m_jobid = 0;

        m_reader.clear();
        m_pending.clear();
        m_pendingTotal = 0;
    }

    IOJob::deleteIfNeeded(job);
}

void FileReader::readerStarted()
{
    if (sender() != m_reader)
        return;

    int bsz = m_buffer.size();
    while (bsz + m_pendingTotal < FILEREADERDEVICE_MAX) {
        m_reader.as<FileJob>()->read(FILEREADERDEVICE_READ);
        m_pendingTotal += FILEREADERDEVICE_READ;
        m_pending.push_back(FILEREADERDEVICE_READ);
    }
}

void FileReader::readerError(const QString &message)
{
    QObject* from = sender();
    if (from && from != m_reader)
        return;

    if (!m_pending.isEmpty()) {
        int p = m_pending.front();
        m_pending.pop_front();
        m_pendingTotal -= p;
    }

    qDebug() << "readerError" << message;

    m_atend = true;
}

void FileReader::readerData(QByteArray *data)
{
    QObject* from = sender();
    if (from && from != m_reader) {
        delete data;
        return;
    }

    if (!m_pending.isEmpty()) {
        int p = m_pending.front();
        m_pending.pop_front();
        m_pendingTotal -= p;
    }

    qDebug() << "readerData, m_p is now" << m_pendingTotal;

    m_buffer.add(data);
}

void FileReader::readerAtEnd()
{
    QObject* from = sender();
    if (from && from != m_reader)
        return;

    m_atend = true;
}

bool FileReader::atEnd() const
{
    return (m_atend && m_buffer.isEmpty());
}

qint64 FileReader::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)

    return 0;
}

qint64 FileReader::readData(char *data, qint64 maxlen)
{
    if (m_atend && m_buffer.isEmpty()) {
        close();
        return 0;
    }

    QByteArray dt = m_buffer.read(maxlen);

    if (!dt.isEmpty())
        memcpy(data, dt.constData(), dt.size());

    if (m_atend || !m_reader)
        return dt.size();

    int bsz = m_buffer.size();
    if (bsz + m_pendingTotal < FILEREADERDEVICE_MIN) {
        do {
            m_reader.as<FileJob>()->read(FILEREADERDEVICE_READ);
            m_pendingTotal += FILEREADERDEVICE_READ;
            m_pending.push_back(FILEREADERDEVICE_READ);
        } while (bsz + m_pendingTotal < FILEREADERDEVICE_MAX);
    }

    return dt.size();
}
