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

#ifndef IO_H
#define IO_H

#include <QThread>
#include <QSet>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QCoreApplication>
#include <QVariant>
#include <QAtomicInt>
#include <QEvent>

class IO;

class IOJob : public QObject
{
    Q_OBJECT
public:
    IOJob(QObject* parent = 0);
    ~IOJob();

public slots:
    void stop();

signals:
    void error(const QString& message);
    void started();
    void finished();

protected:
    void moveToOrigin();

private:
    Q_INVOKABLE void stopJob();

private:
    QThread* m_origin;
    IO* m_io;

    friend class IO;
};

class IO : public QThread
{
    Q_OBJECT
public:
    static IO* instance();

    ~IO();

    static void init();

    void stop();
    void startJob(IOJob* job);

protected:
    void run();

signals:
    void error(const QString& message);

private:
    IO(QObject *parent = 0);

    Q_INVOKABLE void stopIO();
    Q_INVOKABLE void startJobIO(IOJob* job);

    void jobStopped(IOJob* job);

private:
    static IO* s_inst;

    QMutex m_mutex;
    QSet<IOJob*> m_jobs;

    friend class IOJob;
};

#endif // IO_H
