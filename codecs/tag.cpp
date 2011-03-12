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

TagGenerator::TagGenerator(const QString &filename, QObject *parent)
    : QObject(parent), m_filename(filename)
{
}

Tag TagGenerator::createTag()
{
    return Tag();
}

Tag TagGenerator::createTag(const QString &filename, const QHash<QString, QVariant> &data)
{
    Tag t;
    t.m_data = data;
    t.m_filename = filename;
    return t;
}

QString TagGenerator::filename() const
{
    return m_filename;
}
