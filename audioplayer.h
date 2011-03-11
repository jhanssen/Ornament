#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include "audiodevice.h"

class CodecDevice;

class AudioPlayer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString filename READ filename WRITE setFilename)
    Q_PROPERTY(AudioDevice* audioDevice READ audioDevice WRITE setAudioDevice)
    Q_ENUMS(State)
public:
    enum State { Stopped, Paused, Playing };

    AudioPlayer(QObject *parent = 0);

    QString filename() const;
    void setFilename(const QString& filename);

    AudioDevice* audioDevice() const;
    void setAudioDevice(AudioDevice* device);

signals:
    void stateChanged(State state);

public slots:
    void play();
    void pause();
    void stop();

private slots:
    void outputStateChanged(QAudio::State state);

private:
    QString mimeType(const QString& filename) const;

private:
    State m_state;

    QString m_filename;
    AudioDevice* m_audio;

    CodecDevice* m_codec;
};

#endif // AUDIOPLAYER_H
