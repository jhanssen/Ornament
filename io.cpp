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

IOJob::IOJob(QObject *parent)
    : QObject(parent), m_no(0), m_origin(QThread::currentThread())
{
}

void IOJob::setJobNumber(int no)
{
    m_no = no;
}

int IOJob::jobNumber() const
{
    return m_no;
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

IO* IO::s_inst = 0;

IO::IO(QObject *parent)
    : QThread(parent), m_jobcount(1)
{
    moveToThread(this);
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

        m_jobs[job->jobNumber()] = job;
        connect(job, SIGNAL(finished()), this, SLOT(localJobFinished()));

        qDebug() << "=== new job success!" << job->jobNumber() << job;

        emit jobCreated(job);

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

    if (!m_jobs.contains(job->jobNumber())) {
        emit error(QLatin1String("Job finished but not in the list of jobs: ") + QLatin1String(job->metaObject()->className()));
        delete job;
        return;
    }

    m_jobs.remove(job->jobNumber());
    job->moveToOrigin();

    emit jobFinished(job);
}

int IO::nextJobNumber()
{
    int no = m_jobcount;
    while (m_jobs.contains(no) || no == 0) // 0 is a reserved number
        ++no;
    m_jobcount = no + 1;
    return no;
}

int IO::startJob(IOJob *job)
{
    int next = nextJobNumber();

    job->setJobNumber(next);
    job->moveToThread(this);
    QCoreApplication::postEvent(this, new JobEvent(job));

    return next;
}

void IO::stop()
{
    StopEvent* event = new StopEvent;
    QCoreApplication::postEvent(this, event);
}
