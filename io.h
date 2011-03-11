#ifndef IO_H
#define IO_H

#include <QThread>
#include <QHash>
#include <QSet>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>

class IO;

class IOJob : public QObject
{
    Q_OBJECT
public:
    IOJob(QObject* parent = 0);

    QString identifier() const;
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

    void setIdentifier(const QString& identifier);
    void setFilename(const QString& filename);
    void setJobNumber(int no);

private:
    QString m_identifier;
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
    void registerJob(const QString& identifier);

    int postJob(const QString& identifier, const QString& filename);

protected:
    void run();
    bool event(QEvent* event);

signals:
    void error(const QString& message);
    void jobStarted(IOJob* job);
    void jobFinished(IOJob* job);

private slots:
    void localJobFinished();

private:
    IO(QObject *parent = 0);

    bool metaObjectForIdentifier(const QString& identifier, QMetaObject& meta);
    int nextJobNumber();

private:
    static IO* s_inst;

    QMutex m_mutex;

    int m_jobcount;
    QHash<QString, QMetaObject> m_registered;
    QHash<int, IOJob*> m_jobs;
};

template<typename T>
void IO::registerJob(const QString& identifier)
{
    QMutexLocker locker(&m_mutex);

    if (m_registered.contains(identifier))
        return;

    m_registered[identifier] = T::staticMetaObject;
}

#endif // IO_H
