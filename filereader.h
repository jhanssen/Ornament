#ifndef FILEREADER_H
#define FILEREADER_H

#include "io.h"
#include "buffer.h"
#include "audioreader.h"
#include <QFile>
#include <QVector>

class FileReader : public AudioReader
{
    Q_OBJECT
public:
    FileReader(QObject* parent = 0);
    FileReader(const QString& filename, QObject* parent = 0);
    ~FileReader();

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
    void ioError(const QString& message);
    void jobCreated(IOJob* job);
    void jobFinished(IOJob* job);

    void readerStarted();
    void readerData(QByteArray* data);
    void readerAtEnd();
    void readerError(const QString& message);

private:
    QString m_filename;
    Buffer m_buffer;

    int m_jobid;
    bool m_atend;
    IOPtr m_reader;

    QVector<int> m_pending;
    qint64 m_pendingTotal;
};

#endif // FILEREADER_H
