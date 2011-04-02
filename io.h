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
