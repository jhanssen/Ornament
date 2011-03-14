#ifndef FILEREADER_H
#define FILEREADER_H

#include "io.h"
#include "buffer.h"
#include <QFile>
#include <QVector>

class FileReader : public IOJob
{
    Q_OBJECT

    Q_PROPERTY(QString filename READ filename WRITE setFilename)
public:
    Q_INVOKABLE FileReader(QObject *parent = 0);

    static void registerType();

    void read(int size);

    QString filename() const;
    void setFilename(const QString& filename);

protected:
    void init();

signals:
    void data(QByteArray* data);
    void atEnd();

private:
    Q_INVOKABLE void readData(int size);

private:
    QString m_filename;
    QFile m_file;
};

class FileReaderDevice : public QIODevice
{
    Q_OBJECT
public:
    FileReaderDevice(QObject* parent = 0);
    FileReaderDevice(const QString& filename, QObject* parent = 0);
    ~FileReaderDevice();

    void setFilename(const QString& filename);

    bool isSequential() const;
    qint64 bytesAvailable() const;

    bool atEnd() const;

    void close();
    bool open(OpenMode mode);

protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

private slots:
    void jobAboutToStart(IOJob* job);
    void jobStarted(IOJob* job);
    void jobFinished(IOJob* job);
    void readerData(QByteArray* data);
    void readerAtEnd();
    void ioError(const QString& message);
    void readerError(const QString& message);

private:
    QString m_filename;
    Buffer m_buffer;

    int m_jobid;
    bool m_atend;
    FileReader* m_reader;

    QVector<int> m_pending;
    qint64 m_pendingTotal;
};

#endif // FILEREADER_H
