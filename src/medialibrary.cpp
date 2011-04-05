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

class MediaLibraryJob : public IOJob
{
    Q_OBJECT
public:
    enum Type { None, ReadLibrary, RequestArtwork, RequestMetaData };

    MediaLibraryJob(MediaLibraryInterface* iface, QObject* parent = 0);

    void setType(Type type);
    void setArgument(const QVariant& arg);

signals:
    void artist(const Artist& a);
    void artwork(const QImage& a);
    void metaData(const Tag& t);

private slots:
    void startJob();

private:
    void readLibrary();
    void requestArtwork(const QString& filename);
    void requestMetaData(const QString& filename);

private:
    MediaLibraryInterface* m_iface;
    Type m_type;
    QVariant m_arg;
};

MediaLibraryJob::MediaLibraryJob(MediaLibraryInterface* iface, QObject *parent)
    : IOJob(parent), m_iface(iface), m_type(None)
{
}

void MediaLibraryJob::setType(Type type)
{
    m_type = type;
}

void MediaLibraryJob::setArgument(const QVariant &arg)
{
    m_arg = arg;
}

void MediaLibraryJob::startJob()
{
    switch (m_type) {
    case ReadLibrary:
        readLibrary();
        break;
    case RequestArtwork:
        requestArtwork(m_arg.toString());
        break;
    case RequestMetaData:
        requestMetaData(m_arg.toString());
        break;
    default:
        break;
    }
}

void MediaLibraryJob::readLibrary()
{
    if (!m_iface)
        return;
    Artist a;
    bool ok = m_iface->readFirstArtist(&a);
    while (ok) {
        emit artist(a);
        a.albums.clear();
        ok = m_iface->readNextArtist(&a);
    }
}

void MediaLibraryJob::requestArtwork(const QString &filename)
{
    if (!m_iface)
        return;
    QImage a;
    m_iface->readArtworkForTrack(filename, &a);
    emit artwork(a);
}

void MediaLibraryJob::requestMetaData(const QString &filename)
{
    if (!m_iface)
        return;
    Tag t;
    m_iface->readMetaDataForTrack(filename, &t);
    emit metaData(t);
}

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
    dir.cd("../medialibraries");
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

    qRegisterMetaType<Artist>("Artist");
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
    MediaLibraryJob* job = new MediaLibraryJob(m_priv->iface);
    job->setType(MediaLibraryJob::ReadLibrary);

    connect(job, SIGNAL(started()), job, SLOT(startJob()));
    connect(job, SIGNAL(finished()), job, SLOT(deleteLater()));
    connect(job, SIGNAL(artist(Artist)), this, SIGNAL(artist(Artist)));
    IO::instance()->startJob(job);
}

void MediaLibrary::requestArtwork(const QString &filename)
{
    MediaLibraryJob* job = new MediaLibraryJob(m_priv->iface);
    job->setType(MediaLibraryJob::RequestArtwork);
    job->setArgument(filename);

    connect(job, SIGNAL(started()), job, SLOT(startJob()));
    connect(job, SIGNAL(finished()), job, SLOT(deleteLater()));
    connect(job, SIGNAL(artwork(QImage)), this, SIGNAL(artwork(QImage)));
    IO::instance()->startJob(job);
}

void MediaLibrary::requestMetaData(const QString &filename)
{
    MediaLibraryJob* job = new MediaLibraryJob(m_priv->iface);
    job->setType(MediaLibraryJob::RequestMetaData);
    job->setArgument(filename);

    connect(job, SIGNAL(started()), job, SLOT(startJob()));
    connect(job, SIGNAL(finished()), job, SLOT(deleteLater()));
    connect(job, SIGNAL(metaData(Tag)), this, SIGNAL(metaData(Tag)));
    IO::instance()->startJob(job);
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
