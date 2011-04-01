#include "io.h"
#include <QDebug>

class JobEvent : public QEvent
{
public:
    enum Type { JobType = QEvent::User + 1 };

    JobEvent(IOJob* job);

    IOJob* m_job;
};

JobEvent::JobEvent(IOJob* job)
    : QEvent(static_cast<QEvent::Type>(JobType)), m_job(job)
{
}

class StopEvent : public QEvent
{
public:
    enum Type { StopType = QEvent::User + 2 };

    StopEvent();
};

StopEvent::StopEvent()
    : QEvent(static_cast<QEvent::Type>(StopType))
{
}

QMutex IOJob::m_deletedMutex;
QSet<IOJob*> IOJob::m_deleted;

IOJob::IOJob(QObject *parent)
    : QObject(parent), m_origin(QThread::currentThread())
{
    QMutexLocker locker(&m_deletedMutex);

    m_deleted.remove(this);
}

IOJob::~IOJob()
{
    QMutexLocker locker(&m_deletedMutex);

    m_deleted.insert(this);
}

bool IOJob::ref()
{
    return m_ref.ref();
}

bool IOJob::deref()
{
    return m_ref.deref();
}

void IOJob::deleteIfNeeded(IOJob *job)
{
    QMutexLocker locker(&m_deletedMutex);

    if (m_deleted.contains(job))
        return;

    if (!job->m_ref) {
        m_deleted.insert(job);

        QMetaObject::invokeMethod(job, "deleteLater");
    }
}

void IOJob::stop()
{
    StopEvent* event = new StopEvent;
    QCoreApplication::postEvent(this, event);
}

bool IOJob::event(QEvent *event)
{
    if (event->type() == static_cast<QEvent::Type>(StopEvent::StopType)) {
        emit finished();
        return true;
    }

    return QObject::event(event);
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
    }
}

bool IO::event(QEvent *event)
{
    if (event->type() == static_cast<QEvent::Type>(JobEvent::JobType)) {
        JobEvent* jobevent = static_cast<JobEvent*>(event);
        IOJob* job = jobevent->m_job;

        m_jobs.insert(job);
        connect(job, SIGNAL(finished()), this, SLOT(localJobFinished()), Qt::DirectConnection);

        qDebug() << "=== new job ready!" << job;

        emit jobReady(job);

        return true;
    } else if (event->type() == static_cast<QEvent::Type>(StopEvent::StopType)) {
        exit();
        return true;
    }
    return QThread::event(event);
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
    job->moveToThread(this);
    QCoreApplication::postEvent(this, new JobEvent(job));
}

void IO::stop()
{
    StopEvent* event = new StopEvent;
    QCoreApplication::postEvent(this, event);
}

IOJobFinisher::IOJobFinisher(QObject *parent)
    : QObject(parent)
{
}

void IOJobFinisher::jobFinished(IOJob* job)
{
    IOJob::deleteIfNeeded(job);
}
