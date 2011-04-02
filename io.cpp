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

#include "io.h"
#include <QDebug>

QMutex IOJob::s_deletedMutex;
QSet<IOJob*> IOJob::s_deleted;

IOJob::IOJob(QObject *parent)
    : QObject(parent), m_origin(QThread::currentThread())
{
    QMutexLocker locker(&s_deletedMutex);

    s_deleted.remove(this);
}

IOJob::~IOJob()
{
    QMutexLocker locker(&s_deletedMutex);

    s_deleted.insert(this);
}

bool IOJob::ref()
{
    return m_ref.ref();
}

bool IOJob::deref()
{
    return m_ref.deref();
}

bool IOJob::deleteIfNeeded(IOJob *job)
{
    QMutexLocker locker(&s_deletedMutex);

    if (s_deleted.contains(job))
        return false;

    if (!job->m_ref) {
        s_deleted.insert(job);

        QMetaObject::invokeMethod(job, "deleteLater");
        return true;
    }

    return false;
}

void IOJob::stop()
{
    QMetaObject::invokeMethod(this, "stopJob");
}

void IOJob::stopJob()
{
    emit finished();
}

void IOJob::moveToOrigin()
{
    moveToThread(m_origin);
}

IOPtr::IOPtr(IOJob* job)
    : m_job(job)
{
    if (m_job)
        m_job->ref();
}

IOPtr::IOPtr(const IOPtr& ptr)
    : m_job(ptr.m_job)
{
    if (m_job)
        m_job->ref();
}

IOPtr::~IOPtr()
{
    if (m_job)
        m_job->deref();
}

IOPtr& IOPtr::operator=(const IOPtr& ptr)
{
    if (m_job)
        m_job->deref();

    m_job = ptr.m_job;
    if (m_job)
        m_job->ref();

    return *this;
}

IOPtr& IOPtr::operator=(IOJob* job)
{
    if (m_job)
        m_job->deref();

    m_job = job;
    if (m_job)
        m_job->ref();

    return *this;
}

bool IOPtr::clear()
{
    if (m_job) {
        m_job->deref();
        m_job = 0;
        return true;
    }

    m_job = 0;
    return false;
}

IOPtr::operator bool() const
{
    return (m_job != 0);
}

IOJob* IOPtr::operator->() const
{
    return m_job;
}

IOJob* IOPtr::operator*() const
{
    return m_job;
}

class IOJobFinisher : public QObject
{
    Q_OBJECT
public:
    IOJobFinisher(QObject* parent = 0);

public slots:
    void jobFinished(IOJob* job);
};

#include "io.moc"

IO* IO::s_inst = 0;

IO::IO(QObject *parent)
    : QThread(parent), m_jobFinisher(new IOJobFinisher)
{
    moveToThread(this);

    connect(this, SIGNAL(jobFinished(IOJob*)), m_jobFinisher, SLOT(jobFinished(IOJob*)), Qt::QueuedConnection);
}

IO::~IO()
{
    delete m_jobFinisher;
}

IO* IO::instance()
{
    return s_inst;
}

void IO::init()
{
    if (!s_inst) {
        s_inst = new IO;
        s_inst->start();

        qRegisterMetaType<IOJob*>("IOJob*");
    }
}

void IO::startJobIO(IOJob *job)
{
    m_jobs.insert(job);
    connect(job, SIGNAL(finished()), this, SLOT(localJobFinished()), Qt::DirectConnection);

    qDebug() << "=== new job ready!" << job;

    emit jobReady(job);
}

void IO::stopIO()
{
    exit();
}

void IO::cleanupIO()
{
    QSet<IOJob*>::iterator it = m_jobs.begin();
    while (it != m_jobs.end()) {
        if (IOJob::deleteIfNeeded(*it))
            it = m_jobs.erase(it);
        else
            ++it;
    }
}

void IO::run()
{
    exec();

    qDeleteAll(m_jobs);
    m_jobs.clear();
}

void IO::localJobFinished()
{
    IOJob* job = qobject_cast<IOJob*>(sender());
    if (!job) {
        emit error(QLatin1String("Job finished but not a job"));
        return;
    }

    if (!m_jobs.contains(job)) {
        emit error(QLatin1String("Job finished but not in the list of jobs: ") + QLatin1String(job->metaObject()->className()));
        delete job;
        return;
    }

    m_jobs.remove(job);
    job->moveToOrigin();

    emit jobFinished(job);
}

void IO::startJob(IOJob *job)
{
    QMetaObject::invokeMethod(this, "startJobIO", Q_ARG(IOJob*, job));
}

void IO::stop()
{
    QMetaObject::invokeMethod(this, "stopIO");
}

void IO::cleanup()
{
    QMetaObject::invokeMethod(this, "cleanupIO");
}

IOJobFinisher::IOJobFinisher(QObject *parent)
    : QObject(parent)
{
}

void IOJobFinisher::jobFinished(IOJob* job)
{
    IOJob::deleteIfNeeded(job);
}
