#include "filereader.h"
#include <QDebug>

#define FILEREADERDEVICE_READ 8192
#define FILEREADERDEVICE_MIN (8192 * 4)
#define FILEREADERDEVICE_MAX (8192 * 10)

FileReader::FileReader(QObject *parent)
    : IOJob(parent)
{
}

void FileReader::start()
{
    QMetaObject::invokeMethod(this, "startJob");
}

void FileReader::startJob()
{
    m_file.setFileName(filename());
    if (!m_file.open(QFile::ReadOnly))
        emit error(QLatin1String("Unable to open file: ") + filename());

    emit started();
}

void FileReader::read(int size)
{
    QMetaObject::invokeMethod(this, "readData", Q_ARG(int, size));
}

void FileReader::readData(int size)
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

QString FileReader::filename() const
{
    return m_filename;
}

void FileReader::setFilename(const QString &filename)
{
    m_filename = filename;
}

FileReaderDevice::FileReaderDevice(QObject *parent)
    : QIODevice(parent), m_jobid(0), m_atend(false), m_reader(0), m_pendingTotal(0)
{
    connect(IO::instance(), SIGNAL(jobCreated(IOJob*)), this, SLOT(jobCreated(IOJob*)));
    connect(IO::instance(), SIGNAL(jobFinished(IOJob*)), this, SLOT(jobFinished(IOJob*)));
    connect(IO::instance(), SIGNAL(error(QString)), this, SLOT(ioError(QString)));
}

FileReaderDevice::FileReaderDevice(const QString &filename, QObject *parent)
    : QIODevice(parent), m_filename(filename), m_jobid(0), m_atend(false), m_reader(0), m_pendingTotal(0)
{
    connect(IO::instance(), SIGNAL(jobCreated(IOJob*)), this, SLOT(jobCreated(IOJob*)));
    connect(IO::instance(), SIGNAL(jobFinished(IOJob*)), this, SLOT(jobFinished(IOJob*)));
    connect(IO::instance(), SIGNAL(error(QString)), this, SLOT(ioError(QString)));
}

FileReaderDevice::~FileReaderDevice()
{
    close();
}

void FileReaderDevice::ioError(const QString &message)
{
    qDebug() << "ioerror" << message;
    close();
}

void FileReaderDevice::setFilename(const QString &filename)
{
    m_filename = filename;
}

bool FileReaderDevice::isSequential() const
{
    return true;
}

qint64 FileReaderDevice::bytesAvailable() const
{
    return m_buffer.size();
}

void FileReaderDevice::close()
{
    if (!isOpen())
        return;

    QIODevice::close();
    m_atend = false;

    m_buffer.clear();
    if (m_reader) {
        m_reader->stop();
        m_jobid = 0;
        m_reader = 0;
        m_pending.clear();
        m_pendingTotal = 0;
    }
}

bool FileReaderDevice::open(OpenMode mode)
{
    if (m_filename.isEmpty())
        return false;
    bool ok = QIODevice::open(mode);
    if (!ok)
        return false;

    m_atend = false;

    m_buffer.clear();
    if (m_reader) {
        m_reader->stop();
        m_jobid = 0;
        m_reader = 0;
        m_pending.clear();
        m_pendingTotal = 0;
    }

    FileReader* job = new FileReader;
    job->setFilename(m_filename);
    m_jobid = IO::instance()->startJob(job);

    return true;
}

void FileReaderDevice::jobCreated(IOJob *job)
{
    if (!m_reader && job->jobNumber() == m_jobid) {
        m_jobid = 0;

        m_reader = static_cast<FileReader*>(job); // not sure if qobject_cast<> is safe to call at this point
        if (!m_reader)
            return;

        connect(m_reader, SIGNAL(started()), this, SLOT(readerStarted()));
        connect(m_reader, SIGNAL(data(QByteArray*)), this, SLOT(readerData(QByteArray*)));
        connect(m_reader, SIGNAL(atEnd()), this, SLOT(readerAtEnd()));
        connect(m_reader, SIGNAL(error(QString)), this, SLOT(readerError(QString)));

        m_reader->start();
    }
}

void FileReaderDevice::jobFinished(IOJob *job)
{
    if ((!m_reader && !m_jobid) || m_reader == job) {
        m_jobid = 0;
        m_reader = 0;
        m_pending.clear();
        m_pendingTotal = 0;

        if (job && m_reader == job)
            job->deleteLater();
    }
}

void FileReaderDevice::readerStarted()
{
    if (sender() != m_reader)
        return;

    int bsz = m_buffer.size();
    while (bsz + m_pendingTotal < FILEREADERDEVICE_MAX) {
        m_reader->read(FILEREADERDEVICE_READ);
        m_pendingTotal += FILEREADERDEVICE_READ;
        m_pending.push_back(FILEREADERDEVICE_READ);
    }
}

void FileReaderDevice::readerError(const QString &message)
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

void FileReaderDevice::readerData(QByteArray *data)
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

void FileReaderDevice::readerAtEnd()
{
    QObject* from = sender();
    if (from && from != m_reader)
        return;

    m_atend = true;
}

bool FileReaderDevice::atEnd() const
{
    return (m_atend && m_buffer.isEmpty());
}

qint64 FileReaderDevice::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)

    return 0;
}

qint64 FileReaderDevice::readData(char *data, qint64 maxlen)
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
            m_reader->read(FILEREADERDEVICE_READ);
            m_pendingTotal += FILEREADERDEVICE_READ;
            m_pending.push_back(FILEREADERDEVICE_READ);
        } while (bsz + m_pendingTotal < FILEREADERDEVICE_MAX);
    }

    return dt.size();
}
