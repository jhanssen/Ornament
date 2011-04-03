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

#include "medialibrary.h"
#include "mediareader.h"
#include <QTimer>
#include <QStringList>
#include <QDir>
#include <QPluginLoader>
#include <QApplication>
#include <QUrl>
#include <QDebug>

class MediaLibraryPrivate : public QObject
{
    Q_OBJECT
public:
    MediaLibraryPrivate(MediaLibrary* parent);
    ~MediaLibraryPrivate();

    void queryInterfaces();

    MediaLibrary* q;
    MediaLibraryInterface* iface;
};

#include "medialibrary.moc"

MediaLibraryPrivate::MediaLibraryPrivate(MediaLibrary *parent)
    : QObject(parent), q(parent), iface(0)
{
}

MediaLibraryPrivate::~MediaLibraryPrivate()
{
}

void MediaLibraryPrivate::queryInterfaces()
{
    QDir dir(QApplication::applicationDirPath());
#ifdef Q_OS_MAC
    dir.cd("../../..");
#endif
    dir.cd("medialibraries");
    QStringList files = dir.entryList(QDir::Files | QDir::Readable);
    foreach(const QString& filename, files) {
        qDebug() << "media library about to be queried" << filename;
        QPluginLoader loader(dir.absoluteFilePath(filename));
        QObject* root = loader.instance();
        if (!root) {
            qDebug() << "... but no root" << loader.errorString();
            continue;
        }
        MediaLibraryInterface* media = qobject_cast<MediaLibraryInterface*>(root);
        if (!media)
            continue;
        qDebug() << "media library detected" << media;
        if (!iface)
            iface = media;
    }
}

MediaLibrary* MediaLibrary::s_inst = 0;

MediaLibrary::MediaLibrary(QObject *parent)
    : QObject(parent), m_priv(new MediaLibraryPrivate(this))
{
    m_priv->queryInterfaces();
}

MediaLibrary::~MediaLibrary()
{
}

void MediaLibrary::init(QObject *parent)
{
    if (!s_inst)
        s_inst = new MediaLibrary(parent);
}

MediaLibrary* MediaLibrary::instance()
{
    return s_inst;
}

void MediaLibrary::readLibrary()
{
    if (!m_priv->iface)
        return;
    Artist a;
    bool ok = m_priv->iface->readFirstArtist(&a);
    while (ok) {
        emit artist(a);
        ok = m_priv->iface->readNextArtist(&a);
    }
}

void MediaLibrary::requestArtwork(const QString &filename)
{
    if (!m_priv->iface)
        return;
    QImage a;
    m_priv->iface->readArtworkForTrack(filename, &a);
    emit artwork(a);
}

void MediaLibrary::requestMetaData(const QString &filename)
{
    if (!m_priv->iface)
        return;
    Tag t;
    m_priv->iface->readMetaDataForTrack(filename, &t);
    emit metaData(t);
}

MediaReader* MediaLibrary::readerForFilename(const QString &filename)
{
    if (!m_priv->iface)
        return 0;
    MediaReaderInterface* readeriface = m_priv->iface->mediaReaderForTrack(filename);
    if (!readeriface)
        return 0;
    MediaReader* reader = new MediaReader(readeriface);
    readeriface->setDataCallback(reader, &MediaReader::dataCallback);
    return reader;
}

QByteArray MediaLibrary::mimeType(const QString &filename) const
{
    if (!m_priv->iface)
        return QByteArray();
    return m_priv->iface->mimeTypeForTrack(filename);
}
