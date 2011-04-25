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

#ifndef CODEC_H
#define CODEC_H

#include <QObject>
#include <QAudioFormat>
#include <QByteArray>

class Codec;

class AudioFileInformation : public QObject
{
    Q_OBJECT
public:
    AudioFileInformation(QObject* parent = 0);

    void setFilename(const QString& filename);
    QString filename() const;

    virtual int length() const = 0;

private:
    QString m_filename;
};

class Codec : public QObject
{
    Q_OBJECT
public:
    enum Status { Ok, NeedInput, Error };

    Codec(QObject* parent = 0);

    virtual bool init() = 0;
    virtual void deinit() = 0;
    virtual void setAudioFormat(const QAudioFormat& format) = 0;

signals:
    void output(QByteArray* data);
    void position(int position);
    void sampleSize(int size);
    void sampleRate(int rate);

public slots:
    virtual void feed(const QByteArray& data, bool end = false) = 0;
    virtual Status decode() = 0;
};

#endif
