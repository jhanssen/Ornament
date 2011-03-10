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

    QStringList devices() const;

    QString device() const;
    bool setDevice(const QString& device);

    QAudioOutput* output() const;

signals:
    void devicesChanged();

private:
    QString m_device;
    QAudioOutput* m_output;
};

#endif // AUDIODEVICE_H
