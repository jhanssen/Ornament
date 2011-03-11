#include "io.h"
#include <QDebug>

JobEvent::JobEvent(const QByteArray &classname, const QString &filename, int no)
    : QEvent(static_cast<QEvent::Type>(JobType)), m_classname(classname), m_filename(filename), m_no(no)
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
    : QObject(parent), m_no(0)
{
}

void IOJob::setFilename(const QString &filename)
{
    m_filename = filename;
}

QString IOJob::filename() const
{
    return m_filename;
}

void IOJob::setJobNumber(int no)
{
    m_no = no;
}

int IOJob::jobNumber() const
{
    return m_no;
}

void IOJob::init()
{
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

bool IO::metaObjectForClassname(const QByteArray& classname, QMetaObject& meta)
{
    QMutexLocker locker(&m_mutex);

    if (!m_registered.contains(classname))
        return false;

    meta = m_registered.value(classname);

    return true;
}

bool IO::event(QEvent *event)
{
    if (event->type() == static_cast<QEvent::Type>(JobEvent::JobType)) {
        JobEvent* jobevent = static_cast<JobEvent*>(event);
        QByteArray classname = jobevent->m_classname;

        QMetaObject metaobj;
        if (!metaObjectForClassname(classname, metaobj)) {
            emit error(QLatin1String("Unknown classname: ") + classname);
            return true;
        }

        if (metaobj.constructorCount() == 0) {
            emit error(QLatin1String("No invokable constructor: ") + classname);
            return true;
        }

        QObject* instance = metaobj.newInstance(Q_ARG(QObject*, 0));
        if (!instance) {
            emit error(QLatin1String("Not able to create new instance: ") + classname);
            return true;
        }

        IOJob* job = qobject_cast<IOJob*>(instance);
        if (!job) {
            emit error(QLatin1String("Instance is not an IOJob: ") + classname + QLatin1String(", ") + QLatin1String(job->metaObject()->className()));
            delete instance;
            return true;
        }

        int no = jobevent->m_no;

        job->setFilename(jobevent->m_filename);
        job->setJobNumber(no);

        m_jobs[no] = job;
        connect(job, SIGNAL(finished()), this, SLOT(localJobFinished()));

        qDebug() << "=== new job success!" << job->jobNumber() << job;

        emit jobAboutToStart(job);
        job->init();
        emit jobStarted(job);

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

    emit jobFinished(job);
    m_jobs.remove(job->jobNumber());
    delete job;
}

int IO::nextJobNumber()
{
    int no = m_jobcount ? m_jobcount : 1; // jobcount == 0 is reserved
    while (m_jobs.contains(no))
        ++no;
    m_jobcount = no + 1;
    return no;
}

void IO::stop()
{
    StopEvent* event = new StopEvent;
    QCoreApplication::postEvent(this, event);
}
