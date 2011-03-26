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
#include "codecs/mad/taglib/taglib/mpeg/id3v2/id3v2frame.h"
#include "codecs/mad/taglib/taglib/mpeg/id3v2/id3v2framefactory.h"
#include <QDebug>

#define INPUT_BUFFER_SIZE (8196 * 5)
#define OUTPUT_BUFFER_SIZE 8196 // Should be divisible by both 4 and 6

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

static signed int MadFixedToInt(mad_fixed_t Fixed)
{
    /* Clipping */
    if(Fixed>=MAD_F_ONE)
        return(INT_MAX);
    if(Fixed<=-MAD_F_ONE)
        return(-INT_MAX);

    /* Conversion. */
    Fixed=Fixed>>(MAD_F_FRACBITS-23);
    return((signed int)Fixed);
}

CodecMad::CodecMad(QObject *parent)
    : Codec(parent), m_buffer(0)
{
}

CodecMad::~CodecMad()
{
    delete[] m_buffer;
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

    // ### need to take m_format more into account here
    if (format.sampleSize() == 24)
        decodeFunc = &CodecMad::decode24;
    else
        decodeFunc = &CodecMad::decode16;

    return true;
}

void CodecMad::decode16(QByteArray** out, char** outptr, char** outend, int* outsize)
{
    signed short sample;
    for (unsigned short i = 0; i < m_synth.pcm.length; ++i) {
        sample = MadFixedToSshort(m_synth.pcm.samples[0][i]);
        *((*outptr)++) = sample & 0xff;
        *((*outptr)++) = sample >> 8;

        if (MAD_NCHANNELS(&m_frame.header) > 1)
            sample = MadFixedToSshort(m_synth.pcm.samples[1][i]);

        *((*outptr)++) = sample & 0xff;
        *((*outptr)++) = sample >> 8;

        *outsize += 4;

        if (*outptr == *outend) {
            emit output(*out);

            *out = new QByteArray(OUTPUT_BUFFER_SIZE, '\0');
            *outptr = (*out)->data();
            *outend = *outptr + OUTPUT_BUFFER_SIZE;
            *outsize = 0;
        }
    }
}

void CodecMad::decode24(QByteArray** out, char** outptr, char** outend, int* outsize)
{
    signed int sample;
    for (unsigned short i = 0; i < m_synth.pcm.length; ++i) {
        sample = MadFixedToInt(m_synth.pcm.samples[0][i]);
        *((*outptr)++) = sample & 0xff;
        *((*outptr)++) = (sample >> 8) & 0xff;
        *((*outptr)++) = sample >> 16;

        if (MAD_NCHANNELS(&m_frame.header) > 1)
            sample = MadFixedToInt(m_synth.pcm.samples[1][i]);

        *((*outptr)++) = sample & 0xff;
        *((*outptr)++) = (sample >> 8) & 0xff;
        *((*outptr)++) = sample >> 16;

        *outsize += 6;

        if (*outptr == *outend) {
            emit output(*out);

            *out = new QByteArray(OUTPUT_BUFFER_SIZE, '\0');
            *outptr = (*out)->data();
            *outend = *outptr + OUTPUT_BUFFER_SIZE;
            *outsize = 0;
        }
    }
}

void CodecMad::feed(const QByteArray& data, bool end)
{
    static int totalpushed = 0;
    totalpushed += data.size();
    qDebug() << "total pushed" << totalpushed;

    qDebug() << "had" << m_data.size() << "bytes already before pushing" << data.size();
    m_data += data;

    size_t rem = 0, copylen;

    if (m_stream.buffer == NULL || m_stream.error == MAD_ERROR_BUFLEN) {
        if (m_stream.next_frame != NULL) {
            rem = m_stream.bufend - m_stream.next_frame;
            memmove(m_buffer, m_stream.next_frame, rem);
        }

        qDebug() << "rem?" << rem;

        copylen = qMin(INPUT_BUFFER_SIZE - rem, static_cast<long unsigned int>(m_data.size()));
        memcpy(m_buffer + rem, m_data.constData(), copylen);
        m_data = m_data.mid(copylen);

        if (end) {
            memset(m_buffer + rem + copylen, 0, MAD_BUFFER_GUARD);
            copylen += MAD_BUFFER_GUARD;
        }

        qDebug() << "pushing" << copylen + rem << "bytes to mad";

        mad_stream_buffer(&m_stream, m_buffer, copylen + rem);
        m_stream.error = static_cast<mad_error>(0);
    }
}

CodecMad::Status CodecMad::decode()
{
    for (;;) {
        if (mad_header_decode(&m_frame.header, &m_stream)) {
            if (MAD_RECOVERABLE(m_stream.error)) {
                if (m_stream.error == MAD_ERROR_LOSTSYNC) {
                    TagLib::ID3v2::Header header;
                    uint size = (int)(m_stream.bufend - m_stream.this_frame);
                    if (size >= header.size()) {
                        header.setData(TagLib::ByteVector(reinterpret_cast<const char*>(m_stream.this_frame), header.size()));
                        uint tagsize = header.tagSize();
                        if (tagsize > 0)
                            mad_stream_skip(&m_stream, tagsize);
                    }

                    continue;
                }
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
        break;
    }

    mad_timer_add(&m_timer, m_frame.header.duration);

    if (mad_frame_decode(&m_frame, &m_stream)) {
        if (MAD_RECOVERABLE(m_stream.error))
            return Ok;
        else if (m_stream.error == MAD_ERROR_BUFLEN || m_stream.error == MAD_ERROR_BUFPTR)
            return NeedInput;
        else
            return Error;
    }

    QByteArray* out = new QByteArray(OUTPUT_BUFFER_SIZE, '\0');
    char* outptr = out->data();
    char* outend = outptr + OUTPUT_BUFFER_SIZE;
    int outsize = 0;

    mad_synth_frame(&m_synth, &m_frame);

    (this->*decodeFunc)(&out, &outptr, &outend, &outsize);

    if (outsize > 0) {
        out->truncate(outsize);
        emit output(out);

        return Ok;
    } else
        delete out;

    return Error;
}
