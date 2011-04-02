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

#include "audiodevice.h"
#include <QAudioDeviceInfo>

AudioDevice::AudioDevice(QObject *parent) :
    QObject(parent), m_output(0)
{
}

AudioDevice::~AudioDevice()
{
    delete m_output;
}

QStringList AudioDevice::devices() const
{
    QStringList ret;

    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    foreach(const QAudioDeviceInfo& device, devices) {
        ret << device.deviceName();
    }

    return ret;
}

QString AudioDevice::device() const
{
    return m_device;
}

QAudioOutput* AudioDevice::output() const
{
    return m_output;
}

void AudioDevice::createOutput()
{
    delete m_output;
    m_output = 0;

    if (m_device.isEmpty())
        return;

    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    foreach(const QAudioDeviceInfo& dev, devices) {
        if (m_device == dev.deviceName()) {
            m_output = new QAudioOutput(dev, dev.preferredFormat());
            m_output->setBufferSize(16384 * 4);
            return;
        }
    }
}

bool AudioDevice::setDevice(const QString &device)
{
    if (m_device == device)
        return true;

    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    foreach(const QAudioDeviceInfo& dev, devices) {
        if (device == dev.deviceName()) {
            m_device = device;
            delete m_output;
            m_output = 0;
            return true;
        }
    }
    return false;
}
