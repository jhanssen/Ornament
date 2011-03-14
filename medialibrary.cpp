#include "medialibrary.h"

MediaLibrary* MediaLibrary::s_inst = 0;

MediaLibrary::MediaLibrary(QObject *parent) :
    QObject(parent)
{
}

MediaLibrary* MediaLibrary::instance()
{
    return s_inst;
}

void MediaLibrary::init(QObject *parent)
{
    if (!s_inst)
        s_inst = new MediaLibrary(parent);
}

QStringList MediaLibrary::paths()
{
    return m_paths;
}

void MediaLibrary::setPaths(const QStringList &paths)
{
    m_paths = paths;
}

void MediaLibrary::addPath(const QString &path)
{
    m_paths.append(path);
}

void MediaLibrary::update()
{
}

void MediaLibrary::readLibrary()
{
}

void MediaLibrary::requestTag(const QString &filename)
{
}

void MediaLibrary::setTag(const QString &filename, const Tag &tag)
{
}
