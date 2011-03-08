#include "codecdevice.h"
#include <QApplication>
#include <QDebug>

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
    connect(m_codec, SIGNAL(output(QByteArray)), this, SLOT(codecOutput(QByteArray)));
    connect(m_codec, SIGNAL(needsInput()), this, SLOT(codecNeedsInput()), Qt::QueuedConnection);
}

void CodecDevice::fillBuffer() const
{
    CodecDevice* that = const_cast<CodecDevice*>(this);
    bool ok = that->m_codec->decode();
    if (!ok)
        that->codecNeedsInput();
}

qint64 CodecDevice::bytesAvailable() const
{
    qDebug() << "hel lo?";
    if (m_decoded.isEmpty())
        fillBuffer();

    return m_decoded.size();
}

qint64 CodecDevice::readData(char *data, qint64 maxlen)
{
    qDebug() << "we go?";
    if (m_decoded.isEmpty())
        fillBuffer();

    qint64 toread = qMin(maxlen, static_cast<qint64>(m_decoded.size()));
    if (toread == 0)
        return 0;

    qDebug() << toread << m_decoded.size();

    memcpy(data, m_decoded.constData(), toread);
    m_decoded = m_decoded.mid(toread);

    return toread;
}

qint64 CodecDevice::writeData(const char *data, qint64 len)
{
    return -1;
}

void CodecDevice::codecNeedsInput()
{
    static bool end = false;
    if (end)
        return;
    QByteArray input = m_input->read(8192);
    qDebug() << "feeding" << input.size();
    end = (input.size() < 8192);
    m_codec->feed(input, end);
    qDebug() << "feed complete, decoding";
    bool cont;
    do {
        cont = m_codec->decode();
    } while (cont);
    qDebug() << "decode complete";
    //if (end)
    //    qApp->quit();
}

void CodecDevice::codecOutput(const QByteArray &output)
{
    m_decoded += output;
}
