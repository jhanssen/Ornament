#ifndef TAG_H
#define TAG_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QVariant>

class MediaJob;

class Tag
{
public:
    Tag();

    QString filename() const;

    QList<QString> keys() const;
    QVariant data(const QString& key) const;

private:
    Tag(const QString& filename);

    QString m_filename;
    QHash<QString, QVariant> m_data;

    friend class MediaJob;
};

Q_DECLARE_METATYPE(Tag)

#endif
