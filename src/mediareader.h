/*
    Ornament - A cross plaform audio player
    Copyright (C) 2011  Jan Erik Hanssen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef S3READER_H
#define S3READER_H

#include <QIODevice>
#include <QtPlugin>
#include "buffer.h"
#include "io.h"

class QThread;
class MediaLibrary;
class MediaReaderJob;
class MediaReaderInterface;

class MediaReader : public QIODevice
{
    Q_OBJECT
public:
    MediaReader(MediaReaderInterface* interface, QObject *parent = 0);
    ~MediaReader();

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
    void jobStarted();
    void jobFinished();

    void readerData(QByteArray* data);
    void readerAtEnd();

private:
    void dataCallback();

private:
    QString m_filename;
    Buffer m_buffer;

    MediaReaderJob* m_reader;
    MediaReaderInterface* m_iface;

    bool m_atend;

    bool m_requestedData;

    friend class MediaLibrary;
};

typedef void (MediaReader::*MediaReaderCallback)();

class MediaReaderInterface
{
public:
    virtual ~MediaReaderInterface() {}

    virtual bool open() = 0;

    virtual bool atEnd() const = 0;
    virtual QByteArray readData(qint64 length) = 0;

    virtual void pause() = 0;
    virtual void resume(qint64 position) = 0;

    virtual void setTargetThread(QThread* thread) = 0;
    virtual void setDataCallback(MediaReader* reader, MediaReaderCallback callback) = 0;
};

#endif // S3READER_H
