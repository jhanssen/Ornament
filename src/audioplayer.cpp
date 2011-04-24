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

#include "audioplayer.h"
#include "codecdevice.h"
#include "medialibrary.h"
#include "mediareader.h"
#include "codecs/codec.h"
#include "codecs/codecs.h"
#include "medialibrary.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QWidget>
#include <QDebug>

AudioPlayer::AudioPlayer(QObject *parent) :
    QObject(parent), m_state(Stopped), m_audio(0), m_outputDevice(0), m_codec(0)
{
    qRegisterMetaType<State>("State");

    AudioImageProvider::setCurrentAudioPlayer(this);

    connect(MediaLibrary::instance(), SIGNAL(artwork(QImage)), this, SLOT(artworkReady(QImage)));

    qDebug() << "constructing audioplayer" << this;

    m_fillTimer.setInterval(100);
    m_fillTimer.setSingleShot(false);
    connect(&m_fillTimer, SIGNAL(timeout()), this, SLOT(fillOutputDevice()));
}

AudioPlayer::~AudioPlayer()
{
    qDebug() << "destructing audioplayer" << this;
}

QString AudioPlayer::filename() const
{
    return m_filename;
}

void AudioPlayer::setFilename(const QString &filename)
{
    m_filename = filename;
    emit filenameChanged();
}

AudioDevice* AudioPlayer::audioDevice() const
{
    return m_audio;
}

AudioPlayer::State AudioPlayer::state() const
{
    return m_state;
}

void AudioPlayer::setAudioDevice(AudioDevice *device)
{
    if (m_audio == device)
        return;

    delete m_audio;
    m_audio = device;
}

QImage AudioPlayer::currentArtwork() const
{
    return m_artwork;
}

QString AudioPlayer::windowTitle() const
{
    return QApplication::topLevelWidgets().first()->windowTitle();
}

void AudioPlayer::setWindowTitle(const QString &title)
{
    QApplication::topLevelWidgets().first()->setWindowTitle(title);
}

void AudioPlayer::artworkReady(const QImage &image)
{
    if (image.isNull()) {
        if (m_artworkHash.isEmpty())
            return;

        m_artwork = image;
        m_artworkHash.clear();
        emit artworkAvailable();
        return;
    }

    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(reinterpret_cast<const char*>(image.constBits()), image.byteCount());
    QByteArray result = hash.result();

    if (result == m_artworkHash) {
        m_artwork = image;
        return;
    }

    m_artworkHash = result;
    m_artwork = image;
    emit artworkAvailable();
}

void AudioPlayer::outputStateChanged(QAudio::State state)
{
    switch (state) {
    case QAudio::ActiveState:
    case QAudio::IdleState:
        m_state = Playing;
        break;
    case QAudio::SuspendedState:
        m_state = Paused;
        break;
    case QAudio::StoppedState:
        if (m_codec->isOpen()) {
            m_state = Stopped;

            m_filename.clear();
            emit filenameChanged();
        } else
            m_state = Done;
        break;
    }

    if (state != QAudio::IdleState)
        emit stateChanged();
}

void AudioPlayer::play()
{
    if (!m_audio)
        return;

    if (m_state == Playing)
        stop();

    if (m_state == Stopped || m_state == Done || m_state == Playing) {
        delete m_codec;
        m_codec = 0;

        QByteArray mime = MediaLibrary::instance()->mimeType(m_filename);
        if (mime.isEmpty())
            return;

        m_artwork = QImage();
        MediaLibrary::instance()->requestArtwork(m_filename);

        Codec* codec = Codecs::instance()->createCodec(mime);
        if (!codec)
            return;

        MediaReader* reader = MediaLibrary::instance()->readerForFilename(m_filename);
        if (!reader->open(MediaReader::ReadOnly)) {
            delete codec;
            delete reader;

            return;
        }

        m_audio->createOutput();
        connect(m_audio->output(), SIGNAL(stateChanged(QAudio::State)),
                this, SLOT(outputStateChanged(QAudio::State)));

        codec->init(m_audio->output()->format());

        m_codec = new CodecDevice(this);
        m_codec->setCodec(codec);
        m_codec->setInputReader(reader);

        if (!m_codec->open(CodecDevice::ReadOnly)) {
            delete m_codec;
            m_codec = 0;

            return;
        }

        m_audio->output()->setNotifyInterval(100);
        connect(m_audio->output(), SIGNAL(notify()), this, SLOT(intervalNotified()));

        //m_audio->output()->start(m_codec);
        if (m_outputDevice)
            delete m_outputDevice;
        m_outputDevice = m_audio->output()->start();
        m_outputDevice->setParent(this);
        fillOutputDevice();
        m_fillTimer.start();
    } else if (m_state == Paused) {
        m_codec->resumeReader();
        m_audio->output()->resume();
    }
}

void AudioPlayer::fillOutputDevice()
{
    int outSize = m_audio->output()->bytesFree();
    if (!outSize) {
        return;
    }

    //qDebug() << "filling" << size << outSize << inSize;

    QByteArray data = m_codec->read(outSize);
    m_outputDevice->write(data);

    if (!m_codec->isOpen()) {
        m_audio->output()->stop();
        m_fillTimer.stop();
    }
}

void AudioPlayer::intervalNotified()
{
    qint64 time = m_audio->output()->processedUSecs();
    time /= 1000;
    emit positionChanged(time);
}

void AudioPlayer::pause()
{
    if (m_state != Playing || !m_audio || !m_audio->output())
        return;

    m_audio->output()->suspend();
    m_codec->pauseReader();
}

void AudioPlayer::stop()
{
    if (m_state != Playing || !m_audio || !m_audio->output())
        return;

    m_audio->output()->stop();
    m_codec->pauseReader();
}

AudioPlayer* AudioImageProvider::s_currentPlayer = 0;

AudioImageProvider::AudioImageProvider()
    : QDeclarativeImageProvider(Image)
{
}

void AudioImageProvider::setCurrentAudioPlayer(AudioPlayer* player)
{
    s_currentPlayer = player;
}

QImage AudioImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(id)

    if (!s_currentPlayer)
        return QImage();

    QImage img = s_currentPlayer->currentArtwork();
    if (img.isNull())
        return img;

    *size = img.size();

    if (requestedSize.isValid())
        return img.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return img;
}
