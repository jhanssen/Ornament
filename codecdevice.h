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

#ifndef CODECDEVICE_H
#define CODECDEVICE_H

#include <QIODevice>
#include <QLinkedList>
#include "buffer.h"

class MediaReader;
class Codec;

class CodecDevice : public QIODevice
{
    Q_OBJECT
public:
    CodecDevice(QObject *parent = 0);
    ~CodecDevice();

    bool isSequential() const;
    qint64 bytesAvailable() const;

    void setInputReader(MediaReader* input);
    void setCodec(Codec* codec);

    bool open(OpenMode mode);

    void pauseReader();
    void resumeReader();

protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

private slots:
    void codecOutput(QByteArray* output);

private:
    bool fillBuffer();

private:
    MediaReader* m_input;
    Codec* m_codec;

    Buffer m_decoded;
};

#endif // CODECDEVICE_H
