#include "tag.h"
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v2frame.h>
#include <taglib/attachedpictureframe.h>
#include <QImage>

template<typename T>
void readRegularTag(T* tag, QHash<QString, QVariant>& data)
{
    data[QLatin1String("title")] = QVariant(TStringToQString(tag->title()));
    data[QLatin1String("artist")] = QVariant(TStringToQString(tag->artist()));
    data[QLatin1String("album")] = QVariant(TStringToQString(tag->album()));
    data[QLatin1String("comment")] = QVariant(TStringToQString(tag->comment()));
    data[QLatin1String("genre")] = QVariant(TStringToQString(tag->genre()));
    data[QLatin1String("year")] = QVariant(tag->year());
    data[QLatin1String("track")] = QVariant(tag->track());
}

Tag::Tag()
{
}

Tag::Tag(const QString &filename)
    : m_filename(filename)
{
    TagLib::MPEG::File mpegfile(filename.toLocal8Bit().constData());
    TagLib::ID3v2::Tag* id3v2 = mpegfile.ID3v2Tag();
    if (id3v2 && !id3v2->isEmpty()) {
        readRegularTag(id3v2, m_data);

        int picnum = 0;

        TagLib::ID3v2::FrameList frames = id3v2->frameListMap()["APIC"]; // attached picture
        TagLib::ID3v2::FrameList::ConstIterator it = frames.begin();
        while (it != frames.end()) {
            TagLib::ID3v2::AttachedPictureFrame* apic = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(*it);
            TagLib::ByteVector bytes = apic->picture();
            QImage img = QImage::fromData(reinterpret_cast<const uchar*>(bytes.data()), bytes.size());
            if (!img.isNull()) {
                m_data[QLatin1String("picture") + QString::number(picnum++)] = QVariant(img);
            }
            ++it;
        }

    } else {
        TagLib::FileRef fileref(filename.toLocal8Bit().constData());
        if (fileref.isNull())
            return;
        TagLib::Tag* tag = fileref.tag();
        if (!tag || tag->isEmpty())
            return;

        readRegularTag(tag, m_data);
    }
}

QString Tag::filename() const
{
    return m_filename;
}


QVariant Tag::data(const QString &key) const
{
    if (!m_data.contains(key))
        return QLatin1String("Unknown");

    return m_data.value(key);
}

QList<QString> Tag::keys() const
{
    return m_data.keys();
}

bool Tag::isValid() const
{
    return !m_data.isEmpty();
}
