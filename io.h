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
    bool event(QEvent* event);

    void setJobNumber(int no);
    void moveToOrigin();

private:
    int m_no;
    QThread* m_origin;

    friend class IO;
};

class IO : public QThread
{
    Q_OBJECT
public:
    static IO* instance();

    static void init();

    void stop();

    int startJob(IOJob* job);

protected:
    void run();
    bool event(QEvent* event);

signals:
    void error(const QString& message);
    void jobCreated(IOJob* job);
    void jobFinished(IOJob* job);

private slots:
    void localJobFinished();

private:
    IO(QObject *parent = 0);

    int nextJobNumber();

private:
    static IO* s_inst;

    QMutex m_mutex;

    int m_jobcount;
    QHash<int, IOJob*> m_jobs;
};

#endif // IO_H
