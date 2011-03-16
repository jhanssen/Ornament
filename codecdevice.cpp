#include "codecdevice.h"
#include "codecs/codec.h"
#include <QApplication>
#include <QDebug>

#define CODEC_BUFFER_MIN (16384 * 4)
#define CODEC_BUFFER_MAX (16384 * 50)
#define CODEC_INPUT_READ 8192

CodecDevice::CodecDevice(QObject *parent) :
    QIODevice(parent), m_input(0), m_codec(0)
{
}

CodecDevice::~CodecDevice()
{
    delete m_input;
    delete m_codec;
}

bool CodecDevice::isSequential() const
{
    qDebug() << "seq!";
    return true;
}

void CodecDevice::setInputDevice(QIODevice *input)
{
    m_input = input;
}

void CodecDevice::setCodec(Codec *codec)
{
    if (m_codec)
        disconnect(m_codec, SIGNAL(output(QByteArray*)), this, SLOT(codecOutput(QByteArray*)));
    m_codec = codec;
    connect(m_codec, SIGNAL(output(QByteArray*)), this, SLOT(codecOutput(QByteArray*)));
}

bool CodecDevice::fillBuffer()
{
    if (m_input->atEnd() || !m_input->isOpen())
        return false;

    Codec::Status status = m_codec->decode();
    do {
        if (status == Codec::NeedInput) {
            QByteArray input = m_input->read(CODEC_INPUT_READ);
            if (input.isEmpty())
                break;

            qDebug() << "feeding" << input.size();
            m_codec->feed(input, m_input->atEnd());
            qDebug() << "feed complete, decoding";
        } else if (status == Codec::Error) {
            // ### ouch
            break;
        }

        do {
            status = m_codec->decode();
            qDebug() << "decode status" << status;
        } while (status == Codec::Ok);
    } while (m_decoded.size() < CODEC_BUFFER_MAX);

    return true;
}

bool CodecDevice::open(OpenMode mode)
{
    bool ok = QIODevice::open(mode);
    if (!ok || !m_input || !m_codec)
        return false;

    fillBuffer();

    return true;
}

qint64 CodecDevice::bytesAvailable() const
{
    return m_decoded.size();
}

qint64 CodecDevice::readData(char *data, qint64 maxlen)
{
    qDebug() << "we go?";
    if (m_decoded.size() < CODEC_BUFFER_MIN) {
        if (!fillBuffer() && m_decoded.isEmpty()) {
            qDebug() << "no go :(";
            close();
            return -1;
        }
    }

    qint64 toread = qMin(maxlen, static_cast<qint64>(m_decoded.size()));
    if (toread == 0)
        return 0;

    qDebug() << toread << m_decoded.size();
    QByteArray bufferdata = m_decoded.read(toread);

    memcpy(data, bufferdata.constData(), toread);

    return toread;
}

qint64 CodecDevice::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)

    return -1;
}

void CodecDevice::codecOutput(QByteArray* output)
{
    m_decoded.add(output);
}
