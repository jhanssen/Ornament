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

#include "mediareader.h"
#include "io.h"
#include "buffer.h"
#include <QDebug>

#define MIN_BUFFER_SIZE (8192 * 10)
#define READ_SIZE (8192 * 50)

class MediaReaderJob : public IOJob
{
    Q_OBJECT
public:
    MediaReaderJob(MediaReaderInterface* iface, QObject* parent = 0);
    ~MediaReaderJob();

    void setFilename(const QString& m_filename);

    void readMore();
    void start();

    void pause();
    void resume();

    void dataSignaled();

signals:
    void data(QByteArray* data);
    void atEnd();
    void starving();

private:
    enum State { Reading, Paused } m_state;

    Q_INVOKABLE void startJob();
    Q_INVOKABLE void readMoreData();
    Q_INVOKABLE void setState(int state); // ### Should really use State here

private:
    void readData();
    void startStreaming();

private:
    MediaReaderInterface* m_iface;
    QString m_filename;
    int m_toread;
    qint64 m_position;
};

#include "mediareader.moc"

MediaReaderJob::MediaReaderJob(MediaReaderInterface* iface, QObject *parent)
    : IOJob(parent), m_state(Reading), m_iface(iface), m_toread(0), m_position(0)
{
    iface->setTargetThread(IO::instance());
}

MediaReaderJob::~MediaReaderJob()
{
    delete m_iface;
}

void MediaReaderJob::setFilename(const QString &fn)
{
    m_filename = fn;
}

void MediaReaderJob::start()
{
    QMetaObject::invokeMethod(this, "startJob");
}

void MediaReaderJob::pause()
{
    QMetaObject::invokeMethod(this, "setState", Q_ARG(int, Paused));
}

void MediaReaderJob::resume()
{
    QMetaObject::invokeMethod(this, "setState", Q_ARG(int, Reading));
}

void MediaReaderJob::setState(int state)
{
    if (!m_iface)
        return;

    State oldstate = m_state;
    m_state = static_cast<State>(state);

    if (m_state == oldstate)
        return;

    qDebug() << "reader state changed to" << m_state;

    switch(m_state) {
    case Paused:
        m_iface->pause();
        break;
    case Reading:
        m_toread = 0;
        m_iface->resume(m_position);
        break;
    }
}

void MediaReaderJob::readMore()
{
    QMetaObject::invokeMethod(this, "readMoreData");
}

void MediaReaderJob::startJob()
{
    if (!m_iface)
        return;
    m_iface->open();
}

void MediaReaderJob::dataSignaled()
{
    if (m_toread == 0) {
        emit starving();
        return;
    }

    readData();
}

void MediaReaderJob::readData()
{
    if (!m_iface)
        return;

    QByteArray* d = new QByteArray(m_iface->readData(m_toread));
    if (d->isEmpty()) {
        if (m_iface->atEnd()) {
            delete m_iface;
            m_iface = 0;

            emit atEnd();
            stop();
        }

        delete d;
        return;
    }

    //qDebug() << "reader read" << d->size() << "bytes";

    const int sz = d->size();
    m_toread -= sz;
    emit data(d);

    m_position += sz;

    if (m_iface->atEnd()) {
        qDebug() << "reader finished";

        emit atEnd();
    }
}

void MediaReaderJob::readMoreData()
{
    if (m_state == Paused)
        return;

    m_toread += READ_SIZE;
    readData();
}

MediaReader::MediaReader(MediaReaderInterface* iface, QObject *parent)
    : QIODevice(parent), m_reader(0), m_iface(iface), m_atend(false), m_requestedData(false)
{
    connect(IO::instance(), SIGNAL(error(QString)), this, SLOT(ioError(QString)));
}

MediaReader::~MediaReader()
{
}

void MediaReader::pause()
{
    if (m_reader)
        m_reader->pause();
}

void MediaReader::resume()
{
    if (m_reader) {
        m_reader->resume();
        m_reader->readMore();
        m_requestedData = true;
    }
}

void MediaReader::setFilename(const QString &filename)
{
    m_filename = filename;
}

bool MediaReader::isSequential() const
{
    return true;
}

qint64 MediaReader::bytesAvailable() const
{
    return m_buffer.size();
}

bool MediaReader::atEnd() const
{
    return (m_atend && m_buffer.isEmpty());
}

void MediaReader::close()
{
    if (!isOpen())
        return;

    QIODevice::close();

    m_buffer.clear();
    if (m_reader) {
        m_reader->stop();
        m_reader = 0;
    }
}

bool MediaReader::open(OpenMode mode)
{
    if (isOpen())
        close();

    m_atend = false;

    MediaReaderJob* job = new MediaReaderJob(m_iface);
    job->setFilename(m_filename);

    connect(job, SIGNAL(started()), this, SLOT(jobStarted()));
    connect(job, SIGNAL(finished()), this, SLOT(jobFinished()));

    IO::instance()->startJob(job);
    m_reader = job;

    return QIODevice::open(mode);
}

qint64 MediaReader::readData(char *data, qint64 maxlen)
{
    if (m_atend && m_buffer.isEmpty()) {
        close();
        return 0;
    }

    QByteArray dt = m_buffer.read(maxlen);

    if (!dt.isEmpty())
        memcpy(data, dt.constData(), dt.size());

    if (!m_requestedData && m_buffer.size() < MIN_BUFFER_SIZE && m_reader) {
        qDebug() << "reader buffer low, requesting more";
        m_reader->readMore();
        m_requestedData = true;
    }

    return dt.size();
}

qint64 MediaReader::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)

    return 0;
}

void MediaReader::jobStarted()
{
    connect(m_reader, SIGNAL(data(QByteArray*)), this, SLOT(readerData(QByteArray*)));
    connect(m_reader, SIGNAL(atEnd()), this, SLOT(readerAtEnd()));
    connect(m_reader, SIGNAL(starving()), this, SLOT(readerStarving()));

    m_reader->start();
}

void MediaReader::jobFinished()
{
    IOJob* job = qobject_cast<IOJob*>(sender());
    if (job)
        job->deleteLater();
    if (job == m_reader)
        m_reader = 0;
}

void MediaReader::readerStarving()
{
    if (m_buffer.size() < MIN_BUFFER_SIZE && m_reader) {
        qDebug() << "reader job starvation, requesting more";
        m_reader->readMore();
        m_requestedData = true;
    }
}

void MediaReader::readerData(QByteArray *data)
{
    QObject* from = sender();
    if (from && from != m_reader) {
        delete data;
        return;
    }

    m_buffer.add(data);
    emit readyRead();

    if (m_requestedData && (m_buffer.size() * 2) > MIN_BUFFER_SIZE)
        m_requestedData = false;
}

void MediaReader::readerAtEnd()
{
    QObject* from = sender();
    if (from && from != m_reader)
        return;

    m_atend = true;
}

void MediaReader::ioError(const QString &message)
{
    qDebug() << "reader error" << message;
}

void MediaReader::dataCallback()
{
    Q_ASSERT(QThread::currentThread() == IO::instance());

    m_reader->dataSignaled();
}
