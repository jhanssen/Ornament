#include "trackduration.h"
#include "codecs/mad/mad/include/mad.h"
#include "codecs/mad/taglib/taglib/mpeg/id3v2/id3v2frame.h"
#include "codecs/mad/taglib/taglib/mpeg/id3v2/id3v2framefactory.h"
#include <math.h>
#include <QFile>
#include <QDebug>

#define INPUT_BUFFER_SIZE 8192
#define TAG_MAX_SIZE (1024 * 10000)

static int timerToMs(mad_timer_t* timer)
{
    static double res = (double)MAD_TIMER_RESOLUTION / 1000.;

    int msec = timer->seconds * 1000;
    msec += (int)round((double)timer->fraction / res);
    return msec;
}

TrackDuration::TrackDuration()
{
}

int TrackDuration::duration(const QFileInfo &fileinfo)
{
    QString fn = fileinfo.absoluteFilePath();
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
                    TagLib::ID3v2::Header header;
                    uint size = (uint)(infostream.bufend - infostream.this_frame);
                    if (size >= header.size()) {
                        header.setData(TagLib::ByteVector(reinterpret_cast<const char*>(infostream.this_frame), header.size()));
                        uint tagsize = header.tagSize();
                        if (tagsize > 0 && tagsize < TAG_MAX_SIZE) {
                            mad_stream_skip(&infostream, tagsize);
                            continue;
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
