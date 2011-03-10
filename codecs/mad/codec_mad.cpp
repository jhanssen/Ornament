// Part of this file adheres to the following license

/*--------------------------------------------------------------------------*
 * (c) 2001--2004 Bertrand Petit                                            *
 *                                                                          *
 * Redistribution and use in source and binary forms, with or without       *
 * modification, are permitted provided that the following conditions       *
 * are met:                                                                 *
 *                                                                          *
 * 1. Redistributions of source code must retain the above copyright        *
 *    notice, this list of conditions and the following disclaimer.         *
 *                                                                          *
 * 2. Redistributions in binary form must reproduce the above               *
 *    copyright notice, this list of conditions and the following           *
 *    disclaimer in the documentation and/or other materials provided       *
 *    with the distribution.                                                *
 *                                                                          *
 * 3. Neither the name of the author nor the names of its contributors      *
 *    may be used to endorse or promote products derived from this          *
 *    software without specific prior written permission.                   *
 *                                                                          *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''       *
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED        *
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A          *
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR       *
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,             *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT         *
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF         *
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND      *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,       *
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT       *
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF       *
 * SUCH DAMAGE.                                                             *
 *                                                                          *
 ****************************************************************************/

#include "codec_mad.h"
#include "codecs/mad/id3lib/include/id3/tag.h"
#include <QDebug>

#define INPUT_BUFFER_SIZE (8192 * 5)
#define OUTPUT_BUFFER_SIZE 8192

CodecFactoryMad::CodecFactoryMad()
{
}

Codec* CodecFactoryMad::create(const QString &codec)
{
    if (codec == QLatin1String("audio/mp3"))
        return new CodecMad(codec);
    return 0;
}

static signed short MadFixedToSshort(mad_fixed_t Fixed)
{
    /* Clipping */
    if(Fixed>=MAD_F_ONE)
        return(SHRT_MAX);
    if(Fixed<=-MAD_F_ONE)
        return(-SHRT_MAX);

    /* Conversion. */
    Fixed=Fixed>>(MAD_F_FRACBITS-15);
    return((signed short)Fixed);
}

CodecMad::CodecMad(const QString &codec, QObject *parent)
    : Codec(codec, parent), m_buffer(0)
{
}

bool CodecMad::init(const QAudioFormat& format)
{
    m_format = format;

    qDebug() << "format:";
    qDebug() << format.channelCount();
    qDebug() << format.sampleRate();
    qDebug() << format.sampleSize();
    qDebug() << format.byteOrder();

    mad_stream_init(&m_stream);
    mad_frame_init(&m_frame);
    mad_synth_init(&m_synth);
    mad_timer_reset(&m_timer);

    if (!m_buffer)
        m_buffer = new unsigned char[INPUT_BUFFER_SIZE + MAD_BUFFER_GUARD];

    return true;
}

void CodecMad::feed(const QByteArray& data, bool end)
{
    m_data += data;

    size_t rem = 0, copylen;

    if (m_stream.buffer == NULL || m_stream.error == MAD_ERROR_BUFLEN) {
        if (m_stream.next_frame != NULL) {
            rem = m_stream.bufend - m_stream.next_frame;
            memmove(m_buffer, m_stream.next_frame, rem);
        }

        copylen = qMin(INPUT_BUFFER_SIZE - rem, static_cast<long unsigned int>(m_data.size()));
        memcpy(m_buffer + rem, m_data.constData(), copylen);
        m_data = m_data.mid(copylen);

        if (end) {
            memset(m_buffer + rem + copylen, 0, MAD_BUFFER_GUARD);
            copylen += MAD_BUFFER_GUARD;
        }

        qDebug() << "pushing" << copylen << "bytes to mad";

        mad_stream_buffer(&m_stream, m_buffer, copylen + rem);
        m_stream.error = static_cast<mad_error>(0);
    }
}

CodecMad::Status CodecMad::decode()
{
    if (mad_frame_decode(&m_frame, &m_stream)) {
        if (MAD_RECOVERABLE(m_stream.error)) {
            // good stuff, but we need to return
            qDebug() << "recoverable foo" << m_stream.error;
            return Ok;
        } else {
            if (m_stream.error == MAD_ERROR_BUFLEN
                || m_stream.error == MAD_ERROR_BUFPTR) {
                // this is fine as well
                return NeedInput;
            } else {
                // ### ow, set some error status here!
                qDebug() << "error foo" << m_stream.error;
                return Error;
            }
        }
    }

    mad_timer_add(&m_timer, m_frame.header.duration);

    // ### need to take m_format into account here

    QByteArray* out = new QByteArray(OUTPUT_BUFFER_SIZE, '\0');
    char* outptr = out->data();
    char* outend = outptr + OUTPUT_BUFFER_SIZE;
    int outsize = 0;

    mad_synth_frame(&m_synth, &m_frame);

    signed short sample;
    for (unsigned short i = 0; i < m_synth.pcm.length; ++i) {
        sample = MadFixedToSshort(m_synth.pcm.samples[0][i]);
        *(outptr++) = sample & 0xff;
        *(outptr++) = sample >> 8;

        if (MAD_NCHANNELS(&m_frame.header) > 1)
            sample = MadFixedToSshort(m_synth.pcm.samples[1][i]);

        *(outptr++) = sample & 0xff;
        *(outptr++) = sample >> 8;

        outsize += 4;

        if (outptr == outend) {
            emit output(out);

            out = new QByteArray(OUTPUT_BUFFER_SIZE, '\0');
            outptr = out->data();
            outend = outptr + OUTPUT_BUFFER_SIZE;
            outsize = 0;
        }
    }

    if (outsize > 0) {
        out->truncate(outsize);
        emit output(out);
    } else
        delete out;

    return Ok;
}

Tag* CodecMad::tag(const QString &filename) const
{
    return new TagMad(filename);
}

TagMad::TagMad(const QString &filename)
    : Tag(filename)
{
    ID3_Tag tag(filename.toLocal8Bit().constData());

    ID3_Tag::Iterator* iter = tag.CreateIterator();
    ID3_Frame* myFrame = NULL;
    while (NULL != (myFrame = iter->GetNext())) {
        QString key = QString::fromUtf8(myFrame->GetTextID());
        QList<QVariant> vals;

        ID3_Frame::Iterator* fiter = myFrame->CreateIterator();
        ID3_Field* myField = NULL;
        while (NULL != (myField = fiter->GetNext())) {
            ID3_FieldType type = myField->GetType();
            switch (type) {
            case ID3FTY_INTEGER:
                vals.append(QVariant(myField->Get()));
                break;
            case ID3FTY_TEXTSTRING:
                vals.append(QVariant(QString::fromUtf16(myField->GetRawUnicodeText())));
                break;
            case ID3FTY_BINARY:
                vals.append(QVariant(QByteArray(reinterpret_cast<const char*>(myField->GetRawBinary()),
                                                myField->BinSize())));
                break;
            default:
                break;
            }
        }
        delete fiter;

        if (!key.isEmpty() && !vals.isEmpty())
            m_data[key] = vals;
    }
    delete iter;
}

QVariant TagMad::data(const QString &key) const
{
    if (!m_data.contains(key))
        return QVariant();

    return m_data.value(key).last();
}

QList<QString> TagMad::keys() const
{
    return m_data.keys();
}
