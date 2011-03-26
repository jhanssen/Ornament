#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QDeclarativeImageProvider>
#include "audiodevice.h"
#include "tag.h"

class CodecDevice;

class AudioPlayer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString filename READ filename WRITE setFilename)
    Q_PROPERTY(AudioDevice* audioDevice READ audioDevice WRITE setAudioDevice)
    Q_PROPERTY(State state READ state)
    Q_PROPERTY(QString windowTitle READ windowTitle WRITE setWindowTitle)
    Q_ENUMS(State)
public:
    enum State { Stopped, Paused, Playing, Done };

    AudioPlayer(QObject *parent = 0);

    QString filename() const;
    void setFilename(const QString& filename);

    AudioDevice* audioDevice() const;
    void setAudioDevice(AudioDevice* device);

    State state() const;

    QString windowTitle() const;
    void setWindowTitle(const QString& title);

    QImage currentArtwork() const;

signals:
    // ### fix this once QML accepts enums as arguments in signals
    void stateChanged();
    void artworkAvailable();
    void durationAvailable(int duration);
    void positionChanged(int position);

public slots:
    void play();
    void pause();
    void stop();

private slots:
    void outputStateChanged(QAudio::State state);
    void artworkReady(const QImage& image);
    void intervalNotified();

private:
    QByteArray mimeType(const QString& filename) const;

private:
    State m_state;

    QString m_filename;
    AudioDevice* m_audio;

    CodecDevice* m_codec;

    QImage m_artwork;
    QByteArray m_artworkHash;
};

class AudioImageProvider : public QDeclarativeImageProvider
{
public:
    AudioImageProvider();

    // ### This is far from perfect but the best I can do without redesigning AudioPlayer
    static void setCurrentAudioPlayer(AudioPlayer* player);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);

private:
    static AudioPlayer* s_currentPlayer;
};

#endif // AUDIOPLAYER_H
