#include "tag.h"

Tag::Tag()
{
}

QString Tag::filename() const
{
    return m_filename;
}


QVariant Tag::data(const QString &key) const
{
    if (!m_data.contains(key))
        return QVariant();

    return m_data.value(key);
}

QList<QString> Tag::keys() const
{
    return m_data.keys();
}

