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

#ifndef PLAYERCODEC_FLAC_H
#define PLAYERCODEC_FLAC_H

#include "codecs/codec.h"
#include <FLAC/all.h>
#include <samplerate.h>

#include <QHash>
#include <QList>

class CodecFlac : public Codec
{
    Q_OBJECT

    Q_CLASSINFO("mimetype", "audio/flac")
public:
    Q_INVOKABLE CodecFlac(QObject* parent = 0);
    ~CodecFlac();

    bool init();
    void deinit();

    void setAudioFormat(const QAudioFormat& format);

public slots:
    void feed(const QByteArray &data, bool end = false);
    Status decode();

private:
    static FLAC__StreamDecoderReadStatus flacRead(const FLAC__StreamDecoder* decoder, FLAC__byte buffer[], size_t* bytes, void* data);
    static FLAC__StreamDecoderSeekStatus flacSeek(const FLAC__StreamDecoder* decoder, FLAC__uint64 offset, void* data);
    static FLAC__StreamDecoderTellStatus flacTell(const FLAC__StreamDecoder* decoder, FLAC__uint64* offset, void* data);
    static FLAC__StreamDecoderLengthStatus flacLength(const FLAC__StreamDecoder* decoder, FLAC__uint64* length, void* data);
    static FLAC__bool flacEof(const FLAC__StreamDecoder* decoder, void* data);
    static FLAC__StreamDecoderWriteStatus flacWrite(const FLAC__StreamDecoder* decoder, const FLAC__Frame* frame, const FLAC__int32* const buffer[], void* data);
    static void flacMeta(const FLAC__StreamDecoder* decoder, const FLAC__StreamMetadata* meta, void* data);
    static void flacError(const FLAC__StreamDecoder* decoder, FLAC__StreamDecoderErrorStatus status, void* data);

private:
    void setFlacMetadata(const FLAC__StreamMetadata* meta);
    bool processAudioData(const FLAC__Frame* frame, const FLAC__int32* const buffer[]);

private:
    QAudioFormat m_format;
    unsigned int m_samplerate;
    bool m_end;
    bool m_infoemitted;
    QByteArray m_data;
    qint64 m_pos;

    FLAC__StreamDecoder* m_decoder;

    SRC_STATE* m_sampleState;
};

#endif
