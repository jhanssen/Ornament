#include "codecdevice.h"
#include <QApplication>
#include <QDebug>

#define CODEC_BUFFER_MIN (16384 * 4)
#define CODEC_BUFFER_MAX (16384 * 10)
#define CODEC_INPUT_READ 8192

Buffer::Buffer()
    : m_size(0)
{
}

void Buffer::add(QByteArray *sub)
{
    m_subs.append(sub);
    m_size += sub->size();
}

int Buffer::size() const
{
    return m_size;
}

bool Buffer::isEmpty() const
{
    return m_size == 0;
}

QByteArray Buffer::read(int size)
{
    QByteArray ret;
    QByteArray* cur;
    QLinkedList<QByteArray*>::Iterator it = m_subs.begin();
    while (it != m_subs.end()) {
        cur = *it;
        if (ret.size() + cur->size() <= size)
            ret += *cur;
        else {
            int rem = size - ret.size();
            ret += cur->left(rem);
            *cur = cur->mid(rem);
            break;
        }
        delete cur;
        it = m_subs.erase(it);
    }
    m_size -= ret.size();
    return ret;
}

CodecDevice::CodecDevice(QObject *parent) :
    QIODevice(parent)
{
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
    m_codec = codec;
    connect(m_codec, SIGNAL(output(QByteArray*)), this, SLOT(codecOutput(QByteArray*)));
}

bool CodecDevice::fillBuffer()
{
    if (m_input->atEnd() || atEnd())
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

            do {
                status = m_codec->decode();
                qDebug() << "decode status" << status;
            } while (status == Codec::Ok);
        }
    } while (m_decoded.size() < CODEC_BUFFER_MAX);

    return true;
}

qint64 CodecDevice::bytesAvailable() const
{
    qDebug() << "hel lo?";
    if (m_decoded.size() < CODEC_BUFFER_MIN) {
        CodecDevice* that = const_cast<CodecDevice*>(this);
        that->fillBuffer();
    }

    return m_decoded.size();
}

qint64 CodecDevice::readData(char *data, qint64 maxlen)
{
    qDebug() << "we go?";
    if (m_decoded.size() < CODEC_BUFFER_MIN) {
        if (!fillBuffer() && m_decoded.isEmpty()) {
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
