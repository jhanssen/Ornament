#include "medialibrary.h"

MediaLibrary* MediaLibrary::s_inst = 0;

MediaLibrary::MediaLibrary(QObject *parent)
    : QObject(parent), m_settings(0)
{
}

MediaLibrary* MediaLibrary::instance()
{
    return s_inst;
}

void MediaLibrary::setSettings(QSettings *settings)
{
    m_settings = settings;
}
