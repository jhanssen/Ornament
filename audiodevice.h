#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H

#include <QObject>
#include <QAudioOutput>

class AudioDevice : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(QStringList devices READ devices)
    Q_PROPERTY(QString device READ device WRITE setDevice)

    AudioDevice(QObject *parent = 0);

    QStringList devices() const;

    QString device() const;
    bool setDevice(const QString& device);

    QAudioOutput* output() const;

private:
    QString m_device;
    QAudioOutput* m_output;
};

#endif // AUDIODEVICE_H
