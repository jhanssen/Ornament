#ifndef S3READER_H
#define S3READER_H

#include "audioreader.h"
#include "buffer.h"
#include "io.h"

class S3Reader : public AudioReader
{
    Q_OBJECT
public:
    S3Reader(QObject *parent = 0);
    ~S3Reader();

    void setFilename(const QString& filename);

    bool isSequential() const;
    qint64 bytesAvailable() const;

    bool atEnd() const;

    void close();
    bool open(OpenMode mode);

    void pause();
    void resume();

protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

private slots:
    void ioError(const QString& message);
    void jobCreated(IOJob* job);
    void jobFinished(IOJob* job);

    void readerData(QByteArray* data);
    void readerAtEnd();

private:
    QString m_filename;
    Buffer m_buffer;

    int m_jobid;
    IOPtr m_reader;

    bool m_atend;

    bool m_requestedData;
};

#endif // S3READER_H
