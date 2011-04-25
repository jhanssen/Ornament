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

#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H

#include <QObject>
#include <QAudioOutput>

class AudioDevice : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(QString device READ device WRITE setDevice)
public:
    AudioDevice(QObject *parent = 0);
    ~AudioDevice();

    QStringList devices() const;

    QString device() const;
    bool setDevice(const QString& device);

    QAudioOutput* output() const;
    void createOutput(int samplerate, int samplesize);

signals:
    void devicesChanged();

private:
    QString m_device;
    QAudioOutput* m_output;
};

#endif // AUDIODEVICE_H
