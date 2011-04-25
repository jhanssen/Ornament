#include "codec_flac.h"
#include <QDebug>

CodecFlac::CodecFlac(QObject *parent)
    : Codec(parent), m_samplerate(0), m_end(false), m_infoemitted(false), m_pos(0), m_decoder(0)
{
}

CodecFlac::~CodecFlac()
{
    deinit();
}

FLAC__StreamDecoderReadStatus CodecFlac::flacRead(const FLAC__StreamDecoder* decoder, FLAC__byte buffer[], size_t* bytes, void* data)
{
    qDebug() << "flac read 1" << *bytes;

    CodecFlac* codec = reinterpret_cast<CodecFlac*>(data);
    if (codec->m_data.isEmpty()) {
        *bytes = 0;
        if (codec->m_end)
            return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
        qDebug() << "flac read 2";
        return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
    }

    int num = qMin(*bytes, static_cast<size_t>(codec->m_data.size()));
    qDebug() << "flac read 3" << num;
    memcpy(buffer, codec->m_data.constData(), num);
    if (num == codec->m_data.size())
        codec->m_data.clear();
    else
        codec->m_data = codec->m_data.mid(num);
    *bytes = num;
    codec->m_pos += num;
    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderSeekStatus CodecFlac::flacSeek(const FLAC__StreamDecoder* decoder, FLAC__uint64 offset, void* data)
{
    return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;
}

FLAC__StreamDecoderTellStatus CodecFlac::flacTell(const FLAC__StreamDecoder *decoder, FLAC__uint64 *offset, void *data)
{
    CodecFlac* codec = reinterpret_cast<CodecFlac*>(data);
    *offset = codec->m_pos;
    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus CodecFlac::flacLength(const FLAC__StreamDecoder *decoder, FLAC__uint64 *length, void *data)
{
    return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
}

FLAC__bool CodecFlac::flacEof(const FLAC__StreamDecoder *decoder, void *data)
{
    CodecFlac* codec = reinterpret_cast<CodecFlac*>(data);
    if (codec->m_end && codec->m_data.isEmpty())
        return true;
    return false;
}

FLAC__StreamDecoderWriteStatus CodecFlac::flacWrite(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32* const buffer[], void *data)
{
    CodecFlac* codec = reinterpret_cast<CodecFlac*>(data);
    bool ok = codec->processAudioData(frame, buffer);
    return ok ? FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE : FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
}

void CodecFlac::flacMeta(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *meta, void *data)
{
    CodecFlac* codec = reinterpret_cast<CodecFlac*>(data);
    codec->setFlacMetadata(meta);
}

void CodecFlac::flacError(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *data)
{
    qDebug() << "flac error" << status;
    switch (status) {
    case FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC:
        qDebug() << "  lost sync";
        break;
    case FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER:
        qDebug() << "  bad header";
        break;
    case FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH:
        qDebug() << "  frame crc mismatch";
        break;
    case FLAC__STREAM_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM:
        qDebug() << "  unparseable stream";
        break;
    default:
        qDebug() << "  unknown error";
        break;
    }
}

void CodecFlac::setFlacMetadata(const FLAC__StreamMetadata *meta)
{
    qDebug() << "flac metadata";
    if (meta->type == FLAC__METADATA_TYPE_STREAMINFO) {
        if (m_infoemitted)
            return;
        m_infoemitted = true;
        qDebug() << "  meta" << meta->data.stream_info.bits_per_sample << meta->data.stream_info.sample_rate;
        emit sampleSize(meta->data.stream_info.bits_per_sample);
        emit sampleRate(meta->data.stream_info.sample_rate);
    }
}

bool CodecFlac::processAudioData(const FLAC__Frame *frame, const FLAC__int32 *const buffer[])
{
    if (m_format == QAudioFormat())
        return true;

    const unsigned int size = frame->header.blocksize;
    const unsigned int channels = frame->header.channels;
    const unsigned int bps = frame->header.bits_per_sample;
    const unsigned int rate = frame->header.sample_rate;
    int mul = 0;
    if (bps == 16)
        mul = 2;
    else if (bps == 24)
        mul = 3;
    else
        return false;

    QByteArray* data = new QByteArray(mul * size * 2, Qt::Uninitialized);
    char* dataptr = data->data();

    if (bps == static_cast<unsigned int>(m_format.sampleSize())) {
        if (rate == static_cast<unsigned int>(m_format.sampleRate())) {
            FLAC__int32 sample;
            for (unsigned int i = 0; i < size; ++i) {
                sample = buffer[0][i];
                if (mul == 2) {
                    *(dataptr++) = (sample >> 8) & 0xff;
                    *(dataptr++) = sample & 0xff;
                } else if (mul == 3) {
                    *(dataptr++) = (sample >> 16) & 0xff;
                    *(dataptr++) = (sample >> 8) & 0xff;
                    *(dataptr++) = sample & 0xff;
                }

                if (channels > 1)
                    sample = buffer[1][i];

                if (mul == 2) {
                    *(dataptr++) = (sample >> 8) & 0xff;
                    *(dataptr++) = sample & 0xff;
                } else if (mul == 3) {
                    *(dataptr++) = (sample >> 16) & 0xff;
                    *(dataptr++) = (sample >> 8) & 0xff;
                    *(dataptr++) = sample & 0xff;
                }
            }
        } else {
            qDebug() << "### fix! rate != format rate";
            delete data;
            return false;
        }
    } else {
        qDebug() << "### fix! size != format size";
        delete data;
        return false;
    }

    emit output(data);

    return true;
}

bool CodecFlac::init()
{
    qDebug() << "flac init";

    m_samplerate = 0;
    m_end = false;
    m_infoemitted = false;
    m_format = QAudioFormat();
    m_pos = 0;
    m_data.clear();

    if (m_decoder) {
        FLAC__stream_decoder_finish(m_decoder);
        FLAC__stream_decoder_delete(m_decoder);
    }
    m_decoder = FLAC__stream_decoder_new();
    FLAC__stream_decoder_init_stream(m_decoder, flacRead, flacSeek, flacTell, flacLength, flacEof, flacWrite, flacMeta, flacError, this);

    return true;
}

void CodecFlac::deinit()
{
    if (m_decoder) {
        FLAC__stream_decoder_finish(m_decoder);
        FLAC__stream_decoder_delete(m_decoder);
        m_decoder = 0;
    }
}

void CodecFlac::setAudioFormat(const QAudioFormat &format)
{
    m_format = format;

    qDebug() << "flac format:";
    qDebug() << format.channelCount();
    qDebug() << format.sampleRate();
    qDebug() << format.sampleSize();
    qDebug() << format.byteOrder();
}

void CodecFlac::feed(const QByteArray &data, bool end)
{
    qDebug() << "feed" << data.size() << end;
    m_data += data;
    if (end)
        m_end = end;
}

inline static bool recoverable(FLAC__StreamDecoderState state)
{
    switch (state) {
    case FLAC__STREAM_DECODER_SEARCH_FOR_METADATA:
    case FLAC__STREAM_DECODER_READ_METADATA:
    case FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC:
    case FLAC__STREAM_DECODER_READ_FRAME:
    case FLAC__STREAM_DECODER_END_OF_STREAM:
        return true;
    default:
        break;
    }
    return false;
}

CodecFlac::Status CodecFlac::decode()
{
    qDebug() << "decode 1";
    if (!m_data.isEmpty()) {
        qint64 sz = m_data.size();
        qDebug() << "decode 2" << sz;
        if (!FLAC__stream_decoder_process_single(m_decoder)) {
            qDebug() << "decode 3" << FLAC__stream_decoder_get_state(m_decoder);
            if (!recoverable(FLAC__stream_decoder_get_state(m_decoder))) {
                qDebug() << "decode 3.5 error";
                return Error;
            }
        }
        if (sz == m_data.size()) {
            qDebug() << "decode 4";
            return NeedInput;
        }
        qDebug() << "decode 5";
        return Ok;
    }
    qDebug() << "decode 6";
    return NeedInput;
}
