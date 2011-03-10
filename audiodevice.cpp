#include "audiodevice.h"
#include <QAudioDeviceInfo>

AudioDevice::AudioDevice(QObject *parent) :
    QObject(parent), m_output(0)
{
}

QStringList AudioDevice::devices() const
{
    QStringList ret;

    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    foreach(QAudioDeviceInfo device, devices) {
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

bool AudioDevice::setDevice(const QString &device)
{
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    foreach(QAudioDeviceInfo dev, devices) {
        if (device == dev.deviceName()) {
            m_device = device;
            delete m_output;
            m_output = new QAudioOutput(dev, dev.preferredFormat());
            return true;
        }
    }
    return false;
}