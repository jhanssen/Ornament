#include "audioplayer.h"
#include "codecdevice.h"
#include "filereader.h"
#include "codecs/codec.h"
#include "codecs/codecs.h"
#include "medialibrary.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QWidget>
#include <QDebug>

AudioPlayer::AudioPlayer(QObject *parent) :
    QObject(parent), m_state(Stopped), m_audio(0), m_codec(0)
{
    qRegisterMetaType<State>("State");

    AudioImageProvider::setCurrentAudioPlayer(this);

    connect(MediaLibrary::instance(), SIGNAL(artwork(QImage)), this, SLOT(artworkReady(QImage)));
}

QString AudioPlayer::filename() const
{
    return m_filename;
}

void AudioPlayer::setFilename(const QString &filename)
{
    m_filename = filename;
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
        if (m_codec->isOpen())
            m_state = Stopped;
        else
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

        QByteArray mime = mimeType(m_filename);
        if (mime.isEmpty())
            return;

        m_artwork = QImage();
        MediaLibrary::instance()->requestArtwork(m_filename);

        Codec* codec = Codecs::instance()->createCodec(mime);
        if (!codec)
            return;

        FileReader* file = new FileReader(m_filename);
        if (!file->open(FileReader::ReadOnly)) {
            delete codec;
            delete file;

            return;
        }

        m_audio->createOutput();
        connect(m_audio->output(), SIGNAL(stateChanged(QAudio::State)),
                this, SLOT(outputStateChanged(QAudio::State)));

        codec->init(m_audio->output()->format());

        m_codec = new CodecDevice(this);
        m_codec->setCodec(codec);
        m_codec->setInputDevice(file);

        if (!m_codec->open(CodecDevice::ReadOnly)) {
            delete m_codec;
            m_codec = 0;

            return;
        }

        m_audio->output()->setNotifyInterval(100);
        connect(m_audio->output(), SIGNAL(notify()), this, SLOT(intervalNotified()));

        m_audio->output()->start(m_codec);
    } else if (m_state == Paused) {
        m_audio->output()->resume();
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
}

void AudioPlayer::stop()
{
    if (m_state != Playing || !m_audio || !m_audio->output())
        return;

    m_audio->output()->stop();
}

QByteArray AudioPlayer::mimeType(const QString &filename) const
{
    if (filename.isEmpty())
        return QByteArray();

    int extpos = filename.lastIndexOf(QLatin1Char('.'));
    if (extpos > 0) {
        QString ext = filename.mid(extpos);
        if (ext == QLatin1String(".mp3"))
            return QByteArray("audio/mp3");
    }

    return QByteArray();
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
