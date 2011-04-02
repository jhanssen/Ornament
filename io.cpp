#include "io.h"
#include <QDebug>

IOJob::IOJob(QObject *parent)
    : QObject(parent), m_origin(QThread::currentThread()), m_io(0)
{
}

IOJob::~IOJob()
{
}

void IOJob::stop()
{
    QMetaObject::invokeMethod(this, "stopJob");
}

void IOJob::stopJob()
{
    Q_ASSERT(m_io == thread());

    if (m_io)
        m_io->jobStopped(this);
    emit finished();
}

void IOJob::moveToOrigin()
{
    moveToThread(m_origin);
}

#include "io.moc"

IO* IO::s_inst = 0;

IO::IO(QObject *parent)
    : QThread(parent)
{
    moveToThread(this);
}

IO::~IO()
{
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
    job->m_io = this;

    qDebug() << "=== new job ready!" << job;

    emit job->started();
}

void IO::stopIO()
{
    exit();
}

void IO::run()
{
    exec();

    qDeleteAll(m_jobs);
    m_jobs.clear();
}

void IO::jobStopped(IOJob *job)
{
    if (!m_jobs.contains(job)) {
        emit error(QLatin1String("Job finished but not in the list of jobs: ") + QLatin1String(job->metaObject()->className()));
        delete job;
        return;
    }

    m_jobs.remove(job);
    job->moveToOrigin();
}

void IO::startJob(IOJob *job)
{
    job->moveToThread(this);
    QMetaObject::invokeMethod(this, "startJobIO", Q_ARG(IOJob*, job));
}

void IO::stop()
{
    QMetaObject::invokeMethod(this, "stopIO");
}
