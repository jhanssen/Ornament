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

#include "codecdevice.h"
#include "audioreader.h"
#include "codecs/codec.h"
#include <QApplication>
#include <QDebug>

#define CODEC_BUFFER_MIN (16384 * 4)
#define CODEC_BUFFER_MAX (16384 * 50)
#define CODEC_INPUT_READ 8192

CodecDevice::CodecDevice(QObject *parent)
    : QIODevice(parent), m_input(0), m_codec(0)
{
}

CodecDevice::~CodecDevice()
{
    delete m_input;
    delete m_codec;
}

bool CodecDevice::isSequential() const
{
    return true;
}

void CodecDevice::setInputReader(AudioReader *input)
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

            //qDebug() << "feeding" << input.size();
            m_codec->feed(input, m_input->atEnd());
            //qDebug() << "feed complete, decoding";
        } else if (status == Codec::Error) {
            qDebug() << "codec error";
            break;
        }

        do {
            status = m_codec->decode();
            //qDebug() << "decode status" << status;
        } while (status == Codec::Ok);
    } while (m_decoded.size() < CODEC_BUFFER_MAX);

    return (status != Codec::Error);
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
    //qDebug() << "we go?" << m_decoded.size();
    if (m_decoded.size() < CODEC_BUFFER_MIN) {
        if (!fillBuffer() && m_decoded.isEmpty()) {
            qDebug() << "no go :(";
            close();
            return -1;
        }
    }

    qint64 toread = qMin(maxlen, static_cast<qint64>(m_decoded.size()));
    //qDebug() << "toread" << toread << m_decoded.size();
    if (toread == 0)
        return 0;

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

void CodecDevice::pauseReader()
{
    m_input->pause();
}

void CodecDevice::resumeReader()
{
    m_input->resume();
}
