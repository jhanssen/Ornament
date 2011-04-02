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

#ifndef PLAYERCODECS_H
#define PLAYERCODECS_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QMetaClassInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>

class Codec;
class AudioFileInformation;

class Codecs : public QObject
{
    Q_OBJECT
public:
    static Codecs* instance();
    static void init();

    QList<QByteArray> codecs();

    Codec* createCodec(const QByteArray& mimetype);
    AudioFileInformation* createAudioFileInformation(const QByteArray& mimetype);

    template<typename T>
    void addCodec();

    template<typename T>
    void addAudioFileInformation();

private:
    Codecs(QObject* parent = 0);

    static Codecs* s_inst;

    QMutex m_mutex;

    QHash<QByteArray, QMetaObject> m_codecs;
    QHash<QByteArray, QMetaObject> m_infos;
};

template<typename T>
void Codecs::addCodec()
{
    QMetaObject metaobj = T::staticMetaObject;

    int mimepos = metaobj.indexOfClassInfo("mimetype");
    if (mimepos == -1) {
        qDebug() << "unable to create codec " << metaobj.className();
        return;
    }

    QMetaClassInfo mimeinfo = metaobj.classInfo(mimepos);
    QByteArray mimetype(mimeinfo.value());

    QMutexLocker locker(&m_mutex);
    m_codecs[mimetype] = metaobj;
}

template<typename T>
void Codecs::addAudioFileInformation()
{
    QMetaObject metaobj = T::staticMetaObject;

    int mimepos = metaobj.indexOfClassInfo("mimetype");
    if (mimepos == -1) {
        qDebug() << "unable to create codec " << metaobj.className();
        return;
    }

    QMetaClassInfo mimeinfo = metaobj.classInfo(mimepos);
    QByteArray mimetype(mimeinfo.value());

    QMutexLocker locker(&m_mutex);
    m_infos[mimetype] = metaobj;
}
#endif
