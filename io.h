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
class IOJobFinisher;

class IOJob : public QObject
{
    Q_OBJECT
public:
    IOJob(QObject* parent = 0);
    ~IOJob();

    void stop();

    bool ref();
    bool deref();

    static void deleteIfNeeded(IOJob* job);

signals:
    void error(const QString& message);
    void finished();

protected:
    bool event(QEvent* event);

    void moveToOrigin();

private:
    QThread* m_origin;
    QAtomicInt m_ref;

    static QMutex m_deletedMutex;
    static QSet<IOJob*> m_deleted;

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

    void startJob(IOJob* job);

protected:
    void run();
    bool event(QEvent* event);

signals:
    void error(const QString& message);
    void jobReady(IOJob* job);
    void jobFinished(IOJob* job);

private slots:
    void localJobFinished();

private:
    IO(QObject *parent = 0);

private:
    static IO* s_inst;

    IOJobFinisher* m_jobFinisher;

    QMutex m_mutex;

    QSet<IOJob*> m_jobs;
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
