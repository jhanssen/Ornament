#include "filereader.h"
#include <QDebug>

#define FILEREADERDEVICE_READ 8192
#define FILEREADERDEVICE_MIN (16384 * 4)
#define FILEREADERDEVICE_MAX (16384 * 50)

FileReader::FileReader(QObject *parent)
    : IOJob(parent)
{
}

void FileReader::registerType()
{
    IO::instance()->registerJob<FileReader>(QLatin1String("FileReader"));
}

void FileReader::init()
{
    qDebug() << "=== initing filereader" << filename();

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
    }
}

FileReaderDevice::FileReaderDevice(QObject *parent)
    : QIODevice(parent), m_jobid(0), m_atend(false), m_reader(0)
{
    connect(IO::instance(), SIGNAL(jobStarted(IOJob*)), this, SLOT(jobStarted(IOJob*)));
    connect(IO::instance(), SIGNAL(jobFinished(IOJob*)), this, SLOT(jobFinished(IOJob*)));
    connect(IO::instance(), SIGNAL(error(QString)), this, SLOT(ioError(QString)));
}

FileReaderDevice::FileReaderDevice(const QString &filename, QObject *parent)
    : QIODevice(parent), m_filename(filename), m_jobid(0), m_atend(false), m_reader(0)
{
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
        m_reader->stop();
        m_reader = 0;
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
        m_reader->stop();
        m_reader = 0;
    }

    m_jobid = IO::instance()->postJob(QLatin1String("FileReader"), m_filename);

    return true;
}

void FileReaderDevice::jobStarted(IOJob *job)
{
    qDebug() << "=== jobstarted" << job->jobNumber() << m_jobid;
    if (!m_reader && job->jobNumber() == m_jobid) {
        m_reader = qobject_cast<FileReader*>(job);
        if (!m_reader)
            return;

        connect(m_reader, SIGNAL(data(QByteArray*)), this, SLOT(readerData(QByteArray*)));
        connect(m_reader, SIGNAL(atEnd()), this, SLOT(readerAtEnd()));

        int req = 0;
        do {
            m_reader->read(FILEREADERDEVICE_READ);
            req += FILEREADERDEVICE_READ;
        } while (req < FILEREADERDEVICE_MAX);
    }
}

void FileReaderDevice::jobFinished(IOJob *job)
{
    if (m_reader == job) {
        disconnect(m_reader, SIGNAL(data(QByteArray*)), this, SLOT(readerData(QByteArray*)));
        disconnect(m_reader, SIGNAL(atEnd()), this, SLOT(readerAtEnd()));
        m_reader = 0;
    }
}

void FileReaderDevice::readerData(QByteArray *data)
{
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
        qDebug() << "=== filereaderdevice at end";
        close();
        return 0;
    }

    QByteArray dt = m_buffer.read(maxlen);

    memcpy(data, dt.constData(), dt.size());

    int bsz = m_buffer.size();
    if (bsz < FILEREADERDEVICE_MIN) {
        do {
            m_reader->read(FILEREADERDEVICE_READ);
            bsz += FILEREADERDEVICE_READ;
        } while (bsz < FILEREADERDEVICE_MAX);
    }

    return dt.size();
}
