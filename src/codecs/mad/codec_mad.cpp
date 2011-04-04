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

#include "codec_mad.h"
#include <taglib/id3v2frame.h>
#include <taglib/id3v2framefactory.h>
#include <math.h>
#include <QFile>
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

static int timerToMs(mad_timer_t* timer)
{
    static double res = (double)MAD_TIMER_RESOLUTION / 1000.;

    int msec = timer->seconds * 1000;
    msec += (int)round((double)timer->fraction / res);
    return msec;
}

AudioFileInformationMad::AudioFileInformationMad(QObject *parent)
    : AudioFileInformation(parent)
{
}

int AudioFileInformationMad::length() const
{
    QString fn = filename();
    if (fn.isEmpty())
        return 0;

    QFile file(fn);
    if (!file.open(QFile::ReadOnly))
        return 0;

    mad_stream infostream;
    mad_header infoheader;
    mad_timer_t infotimer;
    mad_stream_init(&infostream);
    mad_header_init(&infoheader);
    mad_timer_reset(&infotimer);

    qint64 r;
    qint64 l = 0;
    unsigned char* buf = new unsigned char[INPUT_BUFFER_SIZE];

    // ### file reading here should possibly be done in the IO thread instead

    while (!file.atEnd()) {
        if (l < INPUT_BUFFER_SIZE) {
            r = file.read(reinterpret_cast<char*>(buf) + l, INPUT_BUFFER_SIZE - l);
            l += r;
        }
        mad_stream_buffer(&infostream, buf, l);
        for (;;) {
            if (mad_header_decode(&infoheader, &infostream)) {
                if (!MAD_RECOVERABLE(infostream.error))
                    break;
                if (infostream.error == MAD_ERROR_LOSTSYNC) {
                    if (qstrncmp(reinterpret_cast<const char*>(infostream.this_frame), "ID3", 3) == 0) {
                        TagLib::ID3v2::Header header;
                        uint size = (uint)(infostream.bufend - infostream.this_frame);
                        if (size >= header.size()) {
                            header.setData(TagLib::ByteVector(reinterpret_cast<const char*>(infostream.this_frame), size));
                            uint tagsize = header.completeTagSize();
                            if (tagsize > 0) {
                                mad_stream_skip(&infostream, tagsize);
                                continue;
                            }
                        }
                    }
                }
                qDebug() << "header decode error while getting file info" << infostream.error;
                continue;
            }
            mad_timer_add(&infotimer, infoheader.duration);
        }
        if (infostream.error != MAD_ERROR_BUFLEN && infostream.error != MAD_ERROR_BUFPTR)
            break;
        memmove(buf, infostream.next_frame, &(buf[l]) - infostream.next_frame);
        l -= (infostream.next_frame - buf);
    }

    mad_stream_finish(&infostream);
    mad_header_finish(&infoheader);
    delete[] buf;

    return timerToMs(&infotimer);
}

CodecMad::CodecMad(QObject *parent)
    : Codec(parent), m_buffer(0)
{
}

CodecMad::~CodecMad()
{
    deinit();
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

void CodecMad::deinit()
{
    delete[] m_buffer;
    m_buffer = 0;

    mad_stream_finish(&m_stream);
    mad_frame_finish(&m_frame);
    mad_synth_finish(&m_synth);
    mad_timer_reset(&m_timer);
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
    //qDebug() << "total pushed" << totalpushed;

    //qDebug() << "had" << m_data.size() << "bytes already before pushing" << data.size();
    m_data += data;

    size_t rem = 0, copylen;

    if (m_stream.buffer == NULL || m_stream.error == MAD_ERROR_BUFLEN) {
        if (m_stream.next_frame != NULL) {
            rem = m_stream.bufend - m_stream.next_frame;
            memmove(m_buffer, m_stream.next_frame, rem);
        }

        //qDebug() << "rem?" << rem;

        copylen = qMin(INPUT_BUFFER_SIZE - rem, static_cast<long unsigned int>(m_data.size()));
        memcpy(m_buffer + rem, m_data.constData(), copylen);
        m_data = m_data.mid(copylen);

        if (end) {
            memset(m_buffer + rem + copylen, 0, MAD_BUFFER_GUARD);
            copylen += MAD_BUFFER_GUARD;
        }

        //qDebug() << "pushing" << copylen + rem << "bytes to mad";

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
                    if (qstrncmp(reinterpret_cast<const char*>(m_stream.this_frame), "ID3", 3) == 0) {
                        TagLib::ID3v2::Header header;
                        uint size = (uint)(m_stream.bufend - m_stream.this_frame);
                        if (size >= header.size()) {
                            header.setData(TagLib::ByteVector(reinterpret_cast<const char*>(m_stream.this_frame), size));
                            uint tagsize = header.completeTagSize();
                            if (tagsize > 0) {
                                mad_stream_skip(&m_stream, tagsize);
                                return Ok;
                            }
                        }
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
        emit position(timerToMs(&m_timer));

        return Ok;
    } else
        delete out;

    return Error;
}
