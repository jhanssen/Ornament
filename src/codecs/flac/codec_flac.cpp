#include "codec_flac.h"
#include <QDebug>

#define FLAC_MIN_BUFFER_SIZE (8192 * 10)

CodecFlac::CodecFlac(QObject *parent)
    : Codec(parent), m_samplerate(0), m_maxframesize(0), m_end(false), m_infoemitted(false), m_pos(0), m_decoder(0), m_sampleState(0)
{
}

CodecFlac::~CodecFlac()
{
    deinit();
}

FLAC__StreamDecoderReadStatus CodecFlac::flacRead(const FLAC__StreamDecoder* decoder, FLAC__byte buffer[], size_t* bytes, void* data)
{
    Q_UNUSED(decoder)

    CodecFlac* codec = reinterpret_cast<CodecFlac*>(data);
    if (codec->m_data.isEmpty()) {
        *bytes = 0;
        return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }

    int num = qMin(*bytes, static_cast<size_t>(codec->m_data.size()));
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
    Q_UNUSED(decoder)
    Q_UNUSED(offset)
    Q_UNUSED(data)

    return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;
}

FLAC__StreamDecoderTellStatus CodecFlac::flacTell(const FLAC__StreamDecoder *decoder, FLAC__uint64 *offset, void *data)
{
    Q_UNUSED(decoder)

    CodecFlac* codec = reinterpret_cast<CodecFlac*>(data);
    *offset = codec->m_pos;
    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus CodecFlac::flacLength(const FLAC__StreamDecoder *decoder, FLAC__uint64 *length, void *data)
{
    Q_UNUSED(decoder)
    Q_UNUSED(length)
    Q_UNUSED(data)

    return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
}

FLAC__bool CodecFlac::flacEof(const FLAC__StreamDecoder *decoder, void *data)
{
    Q_UNUSED(decoder)

    CodecFlac* codec = reinterpret_cast<CodecFlac*>(data);
    if (codec->m_end && codec->m_data.isEmpty())
        return true;
    return false;
}

FLAC__StreamDecoderWriteStatus CodecFlac::flacWrite(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32* const buffer[], void *data)
{
    Q_UNUSED(decoder)

    CodecFlac* codec = reinterpret_cast<CodecFlac*>(data);
    bool ok = codec->processAudioData(frame, buffer);
    return ok ? FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE : FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
}

void CodecFlac::flacMeta(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *meta, void *data)
{
    Q_UNUSED(decoder)

    CodecFlac* codec = reinterpret_cast<CodecFlac*>(data);
    codec->setFlacMetadata(meta);
}

void CodecFlac::flacError(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *data)
{
    Q_UNUSED(decoder)
    Q_UNUSED(data)

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
        m_maxframesize = meta->data.stream_info.max_framesize;
        m_samplerate = meta->data.stream_info.sample_rate;
        qDebug() << "  meta" << meta->data.stream_info.bits_per_sample << meta->data.stream_info.sample_rate;
        emit sampleSize(meta->data.stream_info.bits_per_sample);
        emit sampleRate(m_samplerate);
    }
}

inline bool CodecFlac::resample(FLAC__int32** left, FLAC__int32** right, unsigned int* size, unsigned int bps)
{
    Q_UNUSED(bps)

    if (!m_samplerate)
        return false;

    const unsigned int insize = *size;

    const double ratio = static_cast<double>(m_format.sampleRate()) / static_cast<double>(m_samplerate);
    qDebug() << "resampling. format:" << m_format.sampleRate() << "flac data:" << m_samplerate;
    const int outsize = static_cast<int>(static_cast<double>(insize) * ratio) + 1;
    const int maxinout = qMax(insize, static_cast<const unsigned int>(outsize));

    float* leftfloat = new float[maxinout * 2];
    float* rightfloat = new float[insize];
    src_int_to_float_array(*left, leftfloat, insize);
    src_int_to_float_array(*right, rightfloat, insize);

    float* combinedin = new float[insize * 2];
    for (unsigned int i = 0, j = 0; i < insize; ++i, j += 2) {
        combinedin[j] = leftfloat[i];
        combinedin[j + 1] = rightfloat[i];
    }

    delete[] rightfloat;

    int outrealsize;
    int err;

    SRC_DATA srcdata;
    srcdata.end_of_input = (insize == 0);
    srcdata.input_frames = insize;
    srcdata.output_frames = outsize;
    srcdata.data_in = combinedin;
    srcdata.data_out = leftfloat;
    srcdata.src_ratio = ratio;
    err = src_process(m_sampleState, &srcdata);
    if (err)
        qDebug() << "error when resampling left" << err << src_strerror(err);
    outrealsize = srcdata.output_frames_gen;
    Q_ASSERT(srcdata.input_frames_used == insize);

    delete[] combinedin;

    FLAC__int32* combinedout = new FLAC__int32[outrealsize * 2];
    src_float_to_int_array(leftfloat, combinedout, outrealsize * 2);

    delete[] leftfloat;

    FLAC__int32* newleft = new FLAC__int32[outrealsize];
    FLAC__int32* newright = new FLAC__int32[outrealsize];

    for (int i = 0, j = 0; i < outrealsize; ++i, j += 2) {
        newleft[i] = combinedout[j];
        newright[i] = combinedout[j + 1];
    }

    delete combinedout;

    *left = newleft;
    *right = newright;
    *size = outrealsize;

    return true;
}

static inline void writeAudioData(const FLAC__int32* left, const FLAC__int32* right, unsigned int size, int mul, char* dataptr)
{
    FLAC__int32 sample;
    for (unsigned int i = 0; i < size; ++i) {
        sample = left[i];
        if (mul == 2) {
            *(dataptr++) = sample & 0xff;
            *(dataptr++) = (sample >> 8) & 0xff;
        } else if (mul == 3) {
            *(dataptr++) = sample & 0xff;
            *(dataptr++) = (sample >> 8) & 0xff;
            *(dataptr++) = (sample >> 16) & 0xff;
        }

        if (right)
            sample = right[i];

        if (mul == 2) {
            *(dataptr++) = sample & 0xff;
            *(dataptr++) = (sample >> 8) & 0xff;
        } else if (mul == 3) {
            *(dataptr++) = sample & 0xff;
            *(dataptr++) = (sample >> 8) & 0xff;
            *(dataptr++) = (sample >> 16) & 0xff;
        }
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

    QByteArray* data = 0;

    if (bps == static_cast<unsigned int>(m_format.sampleSize())) {
        if (rate == static_cast<unsigned int>(m_format.sampleRate())) {
            data = new QByteArray(mul * size * 2, Qt::Uninitialized);
            char* dataptr = data->data();
            writeAudioData(buffer[0], (channels > 1 ? buffer[1] : 0), size, mul, dataptr);
        } else {
            FLAC__int32* left = const_cast<FLAC__int32*>(buffer[0]);
            FLAC__int32* right = const_cast<FLAC__int32*>((channels > 1 ? buffer[1] : buffer[0]));
            unsigned int rsize = size;

            bool ok = resample(&left, &right, &rsize, bps);
            data = new QByteArray(mul * rsize * 2, Qt::Uninitialized);
            char* dataptr = data->data();
            writeAudioData(left, right, rsize, mul, dataptr);

            if (ok) {
                delete[] left;
                delete[] right;
            }
        }
    } else {
        qDebug() << "### fix! size != format size";
        return false;
    }

    emit output(data);

    return true;
}

bool CodecFlac::init()
{
    m_samplerate = 0;
    m_maxframesize = 0;
    m_end = false;
    m_infoemitted = false;
    m_format = QAudioFormat();
    m_pos = 0;
    m_data.clear();

    int err;
    m_sampleState = src_new(SRC_SINC_MEDIUM_QUALITY, 2, &err);

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

        src_delete(m_sampleState);
        m_sampleState = 0;
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
    if (static_cast<unsigned int>(m_data.size()) >= (m_maxframesize ? m_maxframesize : FLAC_MIN_BUFFER_SIZE) || m_end) {
        qint64 sz = m_data.size();
        if (sz && FLAC__stream_decoder_get_state(m_decoder) == FLAC__STREAM_DECODER_END_OF_STREAM) {
            qDebug() << "flac flushing";
            FLAC__stream_decoder_flush(m_decoder);
        }
        if (!FLAC__stream_decoder_process_single(m_decoder)) {
            if (!recoverable(FLAC__stream_decoder_get_state(m_decoder)))
                return Error;
        }
        if (sz && sz == m_data.size())
            return NeedInput;
        return Ok;
    }
    return NeedInput;
}
