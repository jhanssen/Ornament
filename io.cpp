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
