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

    Q_PROPERTY(QString filename READ filename WRITE setFilename NOTIFY filenameChanged)
    Q_PROPERTY(AudioDevice* audioDevice READ audioDevice WRITE setAudioDevice)
    Q_PROPERTY(State state READ state)
    Q_PROPERTY(QString windowTitle READ windowTitle WRITE setWindowTitle)
    Q_ENUMS(State)
public:
    enum State { Stopped, Paused, Playing, Done };

    AudioPlayer(QObject *parent = 0);
    ~AudioPlayer();

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
    void positionChanged(int position);
    void filenameChanged();

public slots:
    void play();
    void pause();
    void stop();

private slots:
    void outputStateChanged(QAudio::State state);
    void artworkReady(const QImage& image);
    void intervalNotified();

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
