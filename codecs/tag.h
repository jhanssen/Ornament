#ifndef TAG_H
#define TAG_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QVariant>

class TagGenerator;

class Tag
{
public:
    Tag();

    QString filename() const;

    QList<QString> keys() const;
    QVariant data(const QString& key) const;

private:
    QString m_filename;
    QHash<QString, QVariant> m_data;

    friend class TagGenerator;
};

Q_DECLARE_METATYPE(Tag)

#endif
