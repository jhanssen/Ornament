#include "audioplayer.h"
#include "codecdevice.h"
#include "filereader.h"
#include "codecs/codec.h"
#include "codecs/codecs.h"
#include "medialibrary.h"
#include <QDebug>

AudioPlayer::AudioPlayer(QObject *parent) :
    QObject(parent), m_state(Stopped), m_audio(0), m_codec(0)
{
    qRegisterMetaType<State>("State");

    connect(MediaLibrary::instance(), SIGNAL(tag(Tag)), this, SLOT(tagReady(Tag)));
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

    if (m_audio && m_audio->output())
        connect(m_audio->output(), SIGNAL(stateChanged(QAudio::State)),
                this, SLOT(outputStateChanged(QAudio::State)));
}

void AudioPlayer::tagReady(const Tag &tag)
{
    qDebug() << "tag:" << tag.filename();
    qDebug() << tag.keys();
    qDebug() << tag.data("title").toString();
}

void AudioPlayer::outputStateChanged(QAudio::State state)
{
    State oldstate = m_state;

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

    if (m_state != oldstate)
        emit stateChanged();
}

void AudioPlayer::play()
{
    if (!m_audio || !m_audio->output())
        return;

    if (m_state == Playing)
        stop();

    if (m_state == Stopped || m_state == Done || m_state == Playing) {
        delete m_codec;
        m_codec = 0;

        QByteArray mime = mimeType(m_filename);
        if (mime.isEmpty())
            return;

        MediaLibrary::instance()->requestTag(m_filename);

        Codec* codec = Codecs::instance()->createCodec(mime);
        if (!codec)
            return;

        FileReaderDevice* file = new FileReaderDevice(m_filename);
        if (!file->open(FileReaderDevice::ReadOnly)) {
            delete codec;
            delete file;

            return;
        }

        codec->init(m_audio->output()->format());

        m_codec = new CodecDevice(this);
        m_codec->setCodec(codec);
        m_codec->setInputDevice(file);

        if (!m_codec->open(CodecDevice::ReadOnly)) {
            delete m_codec;
            m_codec = 0;

            return;
        }

        m_audio->output()->start(m_codec);
    } else if (m_state == Paused) {
        m_audio->output()->resume();
    }
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
