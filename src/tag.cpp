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
