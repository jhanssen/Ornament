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

#include "audioreader.h"
#include "buffer.h"
#include "io.h"

class S3ReaderJob;

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
    void jobStarted();
    void jobFinished();

    void readerData(QByteArray* data);
    void readerAtEnd();
    void readerStarving();

private:
    QString m_filename;
    Buffer m_buffer;

    S3ReaderJob* m_reader;

    bool m_atend;

    bool m_requestedData;
};

#endif // S3READER_H
