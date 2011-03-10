#include "audioplayer.h"
#include "codecdevice.h"
#include "codecs/codecs.h"
#include <QFile>
#include <QDebug>

AudioPlayer::AudioPlayer(QObject *parent) :
    QObject(parent), m_state(Stopped), m_audio(0), m_codec(0)
{
    Codecs::init();
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

void AudioPlayer::setAudioDevice(AudioDevice *device)
{
    delete m_audio;
    m_audio = device;
}

void AudioPlayer::play()
{
    if (m_state == Playing || !m_audio || !m_audio->output())
        return;

    else if (m_state == Stopped) {
        delete m_codec;

        QString mime = mimeType(m_filename);
        if (!mime.isEmpty()) {
            Codec* codec = Codecs::create(mime);
            if (!codec)
                return;
            QFile* file = new QFile(m_filename);
            if (!file->open(QFile::ReadOnly)) {
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
        }
    }

    m_state = Playing;
    m_audio->output()->start(m_codec);
}

void AudioPlayer::pause()
{
}

void AudioPlayer::stop()
{
}

QString AudioPlayer::mimeType(const QString &filename) const
{
    if (filename.isEmpty())
        return QString();

    int extpos = filename.lastIndexOf(QLatin1Char('.'));
    if (extpos > 0) {
        QString ext = filename.mid(extpos);
        if (ext == QLatin1String(".mp3"))
            return QLatin1String("audio/mp3");
    }

    return QString();
}
