#include "filereader.h"
#include <QDebug>

#define FILEREADERDEVICE_READ 8192
#define FILEREADERDEVICE_MIN (8192 * 4)
#define FILEREADERDEVICE_MAX (8192 * 10)

FileReader::FileReader(QObject *parent)
    : IOJob(parent)
{
}

void FileReader::registerType()
{
    IO::instance()->registerJob<FileReader>();
}

void FileReader::init()
{
    m_file.setFileName(filename());
    if (!m_file.open(QFile::ReadOnly))
        emit error(QLatin1String("Unable to open file: ") + filename());
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
    connect(IO::instance(), SIGNAL(jobAboutToStart(IOJob*)), this, SLOT(jobAboutToStart(IOJob*)));
    connect(IO::instance(), SIGNAL(jobStarted(IOJob*)), this, SLOT(jobStarted(IOJob*)));
    connect(IO::instance(), SIGNAL(jobFinished(IOJob*)), this, SLOT(jobFinished(IOJob*)));
    connect(IO::instance(), SIGNAL(error(QString)), this, SLOT(ioError(QString)));
}

FileReaderDevice::FileReaderDevice(const QString &filename, QObject *parent)
    : QIODevice(parent), m_filename(filename), m_jobid(0), m_atend(false), m_reader(0), m_pendingTotal(0)
{
    connect(IO::instance(), SIGNAL(jobAboutToStart(IOJob*)), this, SLOT(jobAboutToStart(IOJob*)));
    connect(IO::instance(), SIGNAL(jobStarted(IOJob*)), this, SLOT(jobStarted(IOJob*)));
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

void FileReaderDevice::readerError(const QString &message)
{
    int p = m_pending.front();
    m_pending.pop_front();
    m_pendingTotal -= p;

    qDebug() << "readerError" << message;

    m_atend = true;
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
    QIODevice::close();
    m_atend = false;

    m_buffer.clear();
    if (m_reader) {
        disconnect(m_reader, SIGNAL(data(QByteArray*)), this, SLOT(readerData(QByteArray*)));
        disconnect(m_reader, SIGNAL(atEnd()), this, SLOT(readerAtEnd()));
        disconnect(m_reader, SIGNAL(error(QString)), this, SLOT(readerError(QString)));
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
        disconnect(m_reader, SIGNAL(data(QByteArray*)), this, SLOT(readerData(QByteArray*)));
        disconnect(m_reader, SIGNAL(atEnd()), this, SLOT(readerAtEnd()));
        disconnect(m_reader, SIGNAL(error(QString)), this, SLOT(readerError(QString)));
        m_reader->stop();
        m_reader = 0;
        m_pending.clear();
        m_pendingTotal = 0;
    }

    PropertyHash props;
    props["filename"] = m_filename;
    m_jobid = IO::instance()->postJob<FileReader>(props);

    return true;
}

void FileReaderDevice::jobAboutToStart(IOJob *job)
{
    if (!m_reader && job->jobNumber() == m_jobid) {
        m_reader = static_cast<FileReader*>(job); // not sure if qobject_cast<> is safe to call at this point
        if (!m_reader)
            return;

        connect(m_reader, SIGNAL(data(QByteArray*)), this, SLOT(readerData(QByteArray*)));
        connect(m_reader, SIGNAL(atEnd()), this, SLOT(readerAtEnd()));
        connect(m_reader, SIGNAL(error(QString)), this, SLOT(readerError(QString)));
    }
}

void FileReaderDevice::jobStarted(IOJob *job)
{
    if (job != m_reader)
        return;

    int bsz = m_buffer.size();
    while (bsz + m_pendingTotal < FILEREADERDEVICE_MAX) {
        m_reader->read(FILEREADERDEVICE_READ);
        m_pendingTotal += FILEREADERDEVICE_READ;
        m_pending.push_back(FILEREADERDEVICE_READ);
    }
}

void FileReaderDevice::jobFinished(IOJob *job)
{
    if (m_reader == job) {
        disconnect(m_reader, SIGNAL(data(QByteArray*)), this, SLOT(readerData(QByteArray*)));
        disconnect(m_reader, SIGNAL(atEnd()), this, SLOT(readerAtEnd()));
        disconnect(m_reader, SIGNAL(error(QString)), this, SLOT(readerError(QString)));
        m_reader = 0;
        m_jobid = 0;
        m_pending.clear();
        m_pendingTotal = 0;
    }
}

void FileReaderDevice::readerData(QByteArray *data)
{
    int p = m_pending.front();
    m_pending.pop_front();
    m_pendingTotal -= p;

    qDebug() << "readerData, m_p is now" << m_pendingTotal;

    m_buffer.add(data);
}

void FileReaderDevice::readerAtEnd()
{
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
