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

#ifndef FILEREADER_H
#define FILEREADER_H

#include "io.h"
#include "buffer.h"
#include "audioreader.h"
#include <QFile>
#include <QVector>

class FileJob;

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
    void jobStarted();
    void jobFinished();

    void readerStarted();
    void readerData(QByteArray* data);
    void readerAtEnd();
    void readerError(const QString& message);

private:
    QString m_filename;
    Buffer m_buffer;

    bool m_atend;
    FileJob* m_reader;
    bool m_started;

    QVector<int> m_pending;
    qint64 m_pendingTotal;
};

#endif // FILEREADER_H
