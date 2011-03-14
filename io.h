#ifndef IO_H
#define IO_H

#include <QThread>
#include <QHash>
#include <QSet>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QCoreApplication>
#include <QVariant>
#include <QEvent>

class IO;

typedef QHash<QByteArray, QVariant> PropertyHash;

class IOJob : public QObject
{
    Q_OBJECT
public:
    IOJob(QObject* parent = 0);

    int jobNumber() const;

    void stop();

signals:
    void error(const QString& message);
    void finished();

protected:
    virtual void init();

protected:
    bool event(QEvent* event);

    void setJobNumber(int no);

private:
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
    bool registerJob();

    template<typename T>
    int postJob(const PropertyHash& properties = PropertyHash());

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

    JobEvent(const QByteArray& classname, int no, const PropertyHash& properties);

    QByteArray m_classname;
    int m_no;
    PropertyHash m_properties;
};

template<typename T>
bool IO::registerJob()
{
    QMutexLocker locker(&m_mutex);

    QByteArray classname(T::staticMetaObject.className());
    if (m_registered.contains(classname))
        return false;

    // Check if the class inherits IOJob
    const QMetaObject* super = T::staticMetaObject.superClass();
    while (super) {
        if (qstrcmp(super->className(), "IOJob") == 0)
            break;
        super = super->superClass();
    }
    if (!super)
        return false;

    m_registered[classname] = T::staticMetaObject;
    return true;
}

template<typename T>
int IO::postJob(const PropertyHash& properties)
{
    QByteArray classname(T::staticMetaObject.className());

    int no = nextJobNumber();
    JobEvent* event = new JobEvent(classname, no, properties);
    QCoreApplication::postEvent(this, event);

    //qDebug() << "=== job posted" << no;

    return no;
}

#endif // IO_H
