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

    bool ref();
    bool deref();

    static bool deleteIfNeeded(IOJob* job);

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
    QAtomicInt m_ref;
    IO* m_io;

    static QMutex s_deletedMutex;
    static QSet<IOJob*> s_deleted;

    friend class IO;
};

class IOPtr
{
public:
    IOPtr(IOJob* job = 0);
    IOPtr(const IOPtr& ptr);
    ~IOPtr();

    IOPtr& operator=(const IOPtr& ptr);
    IOPtr& operator=(IOJob* job);

    IOJob* operator->() const;
    IOJob* operator*() const;

    operator bool() const;

    template<typename T>
    T* as() const
    {
        return static_cast<T*>(m_job);
    }

    bool clear();

private:
    IOJob* m_job;
};

class IO : public QThread
{
    Q_OBJECT
public:
    static IO* instance();

    ~IO();

    static void init();

    void stop();
    void cleanup();
    void startJob(IOJob* job);

protected:
    void run();

signals:
    void error(const QString& message);

private:
    IO(QObject *parent = 0);

    Q_INVOKABLE void cleanupIO();
    Q_INVOKABLE void stopIO();
    Q_INVOKABLE void startJobIO(IOJob* job);
    Q_INVOKABLE void deleteJobLater(IOJob* job);

    void jobStopped(IOJob* job);

private:
    static IO* s_inst;

    QMutex m_mutex;
    QSet<IOJob*> m_jobs;

    friend class IOJob;
};

inline bool operator==(const IOPtr& ptr, const QObject* obj)
{
    return (*ptr == obj);
}

inline bool operator==(const QObject* obj, const IOPtr& ptr)
{
    return (obj == *ptr);
}

inline bool operator!=(const IOPtr& ptr, const QObject* obj)
{
    return (*ptr != obj);
}

inline bool operator!=(const QObject* obj, const IOPtr& ptr)
{
    return (obj != *ptr);
}

#endif // IO_H
