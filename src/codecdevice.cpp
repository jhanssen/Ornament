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
#include "mediareader.h"
#include "codecs/codec.h"
#include <QApplication>
#include <QThread>
#include <QEventLoop>
#include <QDebug>

#define CODEC_BUFFER_MIN (16384 * 4)
#define CODEC_BUFFER_MAX (16384 * 50)
#define CODEC_INPUT_READ 8192

class CodecThread : public QThread
{
    Q_OBJECT
public:
    CodecThread(QObject* parent = 0);
    ~CodecThread();

    void setMediaReader(MediaReader* reader);
    void setCodec(Codec* codec);

    void stop();
    void notifyConsumed(int size);

    void pause();
    void resume();

    void setAudioFormat(const QAudioFormat& format);

signals:
    void data(QByteArray* b);
    void atEnd();
    void error();

protected:
    void run();

private slots:
    void emitData(QByteArray* b);

    void fillBuffer();

private:
    Q_INVOKABLE void stopThread();
    Q_INVOKABLE void consumedData(int size);
    Q_INVOKABLE void init();
    Q_INVOKABLE void pauseReader();
    Q_INVOKABLE void resumeReader();
    Q_INVOKABLE void setCodecFormat(const QAudioFormat& format);

private:
    int m_total;
    MediaReader* m_reader;
    Codec* m_codec;

    QThread* m_origin;
};

CodecThread::CodecThread(QObject *parent)
    : QThread(parent), m_total(0), m_reader(0), m_codec(0), m_origin(QThread::currentThread())
{
    moveToThread(this);
}

CodecThread::~CodecThread()
{
    delete m_reader;
    delete m_codec;
}

void CodecThread::setAudioFormat(const QAudioFormat &format)
{
    QMetaObject::invokeMethod(this, "setCodecFormat", Q_ARG(QAudioFormat, format));
}

void CodecThread::setMediaReader(MediaReader *reader)
{
    if (m_reader)
        QMetaObject::invokeMethod(m_reader, "deleteLater");
    m_reader = reader;
    m_reader->moveToThread(this);
    QMetaObject::invokeMethod(this, "init");
}

void CodecThread::setCodec(Codec *codec)
{
    if (m_codec)
        QMetaObject::invokeMethod(m_codec, "deleteLater");
    m_codec = codec;
    m_codec->moveToThread(this);
    QMetaObject::invokeMethod(this, "init");
}

void CodecThread::stop()
{
    QMetaObject::invokeMethod(this, "stopThread");
}

void CodecThread::notifyConsumed(int size)
{
    QMetaObject::invokeMethod(this, "consumedData", Q_ARG(int, size));
}

void CodecThread::pause()
{
    QMetaObject::invokeMethod(this, "pauseReader");
}

void CodecThread::resume()
{
    QMetaObject::invokeMethod(this, "resumeReader");
}

void CodecThread::pauseReader()
{
    if (m_reader)
        m_reader->pause();
}

void CodecThread::resumeReader()
{
    if (m_reader)
        m_reader->resume();
}

void CodecThread::stopThread()
{
    quit();
}

void CodecThread::setCodecFormat(const QAudioFormat &format)
{
    if (m_codec)
        m_codec->setAudioFormat(format);
}

void CodecThread::consumedData(int size)
{
    m_total -= size;
    fillBuffer();
}

void CodecThread::emitData(QByteArray* b)
{
    m_total += b->size();
    emit data(b);
}

void CodecThread::fillBuffer()
{
    if (!m_reader || !m_codec) {
        return;
    }

    if (m_reader->atEnd() || !m_reader->isOpen()) {
        emit atEnd();
        return;
    }

    if (m_total > CODEC_BUFFER_MIN)
        return;

    Codec::Status status = m_codec->decode();
    do {
        if (status == Codec::NeedInput) {
            QByteArray input = m_reader->read(CODEC_INPUT_READ);
            if (input.isEmpty()) {
                break;
            }

            //qDebug() << "feeding" << input.size();
            m_codec->feed(input, m_reader->atEnd());
            //qDebug() << "feed complete, decoding";
        } else if (status == Codec::Error) {
            qDebug() << "codec error";
            break;
        }

        do {
            status = m_codec->decode();
            //qDebug() << "decode status" << status;
        } while (status == Codec::Ok);
    } while (m_total < CODEC_BUFFER_MAX);

    if (status == Codec::Error)
        emit error();
}

void CodecThread::init()
{
    m_total = 0;
    if (m_codec) {
        disconnect(m_codec, SIGNAL(output(QByteArray*)), this, SLOT(emitData(QByteArray*)));
        connect(m_codec, SIGNAL(output(QByteArray*)), this, SLOT(emitData(QByteArray*)));
        fillBuffer();
    }
    if (m_reader) {
        disconnect(m_reader, SIGNAL(readyRead()), this, SLOT(fillBuffer()));
        connect(m_reader, SIGNAL(readyRead()), this, SLOT(fillBuffer()));
    }
}

void CodecThread::run()
{
    init();

    exec();
}

#include "codecdevice.moc"

CodecDevice::CodecDevice(QObject *parent)
    : QIODevice(parent), m_atend(false), m_thread(new CodecThread), m_decodeloop(0), m_samplesize(0), m_samplerate(0)
{
    connect(m_thread, SIGNAL(finished()), this, SLOT(disposeThread()));
    connect(m_thread, SIGNAL(data(QByteArray*)), this, SLOT(codecOutput(QByteArray*)));
    connect(m_thread, SIGNAL(atEnd()), this, SLOT(codecAtEnd()));
    connect(m_thread, SIGNAL(error()), this, SLOT(codecError()));
    m_thread->start();
}

CodecDevice::~CodecDevice()
{
    m_thread->stop();
    m_thread->wait();
    delete m_thread;
}

void CodecDevice::disposeThread()
{
    CodecThread* thread = qobject_cast<CodecThread*>(sender());
    if (!thread)
        return;
    delete thread;
}

bool CodecDevice::isSequential() const
{
    return true;
}

void CodecDevice::setInputReader(MediaReader *input)
{
    m_thread->setMediaReader(input);
}

void CodecDevice::setCodec(Codec *codec)
{
    m_thread->setCodec(codec);
    connect(codec, SIGNAL(sampleRate(int)), this, SLOT(codecSampleRate(int)));
    connect(codec, SIGNAL(sampleSize(int)), this, SLOT(codecSampleSize(int)));
}

bool CodecDevice::open(OpenMode mode)
{
    bool ok = QIODevice::open(mode);
    if (!ok)
        return false;

    m_samplerate = m_samplesize = 0;
    m_atend = false;
    return true;
}

qint64 CodecDevice::bytesAvailable() const
{
    return m_decoded.size();
}

qint64 CodecDevice::readData(char *data, qint64 maxlen)
{
    if (m_decoded.size() < CODEC_BUFFER_MIN) {
        if (!m_atend)
            m_thread->notifyConsumed(0);
        else if (m_decoded.isEmpty()) {
            close();
            return 0;
        }
    }

    qint64 toread = qMin(maxlen, static_cast<qint64>(m_decoded.size()));
    //qDebug() << "toread" << toread << m_decoded.size();
    if (toread == 0)
        return 0;

    QByteArray bufferdata = m_decoded.read(toread);
    memcpy(data, bufferdata.constData(), toread);

    m_thread->notifyConsumed(toread);

    return toread;
}

qint64 CodecDevice::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)

    return -1;
}

void CodecDevice::decodeUntilInformationReceived()
{
    if (m_samplerate && m_samplesize)
        return;

    QEventLoop loop;
    m_decodeloop = &loop;
    loop.exec();
    m_decodeloop = 0;
}

void CodecDevice::codecAtEnd()
{
    m_atend = true;
}

void CodecDevice::codecError()
{
    qDebug() << "codec error";
    m_decoded.clear();
    close();
}

void CodecDevice::codecSampleRate(int rate)
{
    m_samplerate = rate;
    if (m_samplerate && m_samplesize && m_decodeloop)
        m_decodeloop->quit();
}

void CodecDevice::codecSampleSize(int size)
{
    m_samplesize = size;
    if (m_samplerate && m_samplesize && m_decodeloop)
        m_decodeloop->quit();
}

void CodecDevice::codecOutput(QByteArray* output)
{
    m_decoded.add(output);
}

void CodecDevice::pauseReader()
{
    m_thread->pause();
}

void CodecDevice::resumeReader()
{
    m_thread->resume();
}

void CodecDevice::setAudioFormat(const QAudioFormat &format)
{
    m_thread->setAudioFormat(format);
}

int CodecDevice::sampleRate() const
{
    return m_samplerate;
}

int CodecDevice::sampleSize() const
{
    return m_samplesize;
}
