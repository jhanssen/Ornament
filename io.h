#ifndef IO_H
#define IO_H

#include <QThread>
#include <QHash>
#include <QSet>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QCoreApplication>
#include <QEvent>

class IO;

class IOJob : public QObject
{
    Q_OBJECT
public:
    IOJob(QObject* parent = 0);

    QString filename() const;
    int jobNumber() const;

    void stop();

signals:
    void error(const QString& message);
    void finished();

protected:
    // This is probably not needed, can do this in the c'tor instead?
    virtual void init();

protected:
    bool event(QEvent* event);

    void setFilename(const QString& filename);
    void setJobNumber(int no);

private:
    QString m_filename;
    int m_no;

    friend class IO;
};

class IO : public QThread
{
    Q_OBJECT
public:
    static IO* instance();

    static void init();

    void stop();

    template<typename T>
    void registerJob();

    template<typename T>
    int postJob(const QString& filename);

protected:
    void run();
    bool event(QEvent* event);

signals:
    void error(const QString& message);
    void jobAboutToStart(IOJob* job);
    void jobStarted(IOJob* job);
    void jobFinished(IOJob* job);

private slots:
    void localJobFinished();

private:
    IO(QObject *parent = 0);

    bool metaObjectForClassname(const QByteArray& classname, QMetaObject& meta);
    int nextJobNumber();

private:
    static IO* s_inst;

    QMutex m_mutex;

    int m_jobcount;
    QHash<QByteArray, QMetaObject> m_registered;
    QHash<int, IOJob*> m_jobs;
};

class JobEvent : public QEvent
{
public:
    enum Type { JobType = QEvent::User + 1 };

    JobEvent(const QByteArray& classname, const QString& filename, int no);

    QByteArray m_classname;
    QString m_filename;
    int m_no;
};

template<typename T>
void IO::registerJob()
{
    QMutexLocker locker(&m_mutex);

    QByteArray classname(T::staticMetaObject.className());
    if (m_registered.contains(classname))
        return;

    m_registered[classname] = T::staticMetaObject;
}

template<typename T>
int IO::postJob(const QString &filename)
{
    QByteArray classname(T::staticMetaObject.className());

    int no = nextJobNumber();
    JobEvent* event = new JobEvent(classname, filename, no);
    QCoreApplication::postEvent(this, event);

    //qDebug() << "=== job posted" << no;

    return no;
}

#endif // IO_H
