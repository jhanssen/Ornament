#ifndef TAG_H
#define TAG_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QVariant>

class TagGenerator;
class TagJob;

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

class TagGenerator : public QObject
{
    Q_OBJECT
public:
    TagGenerator(const QString& filename, QObject* parent = 0);

protected:
    // This method will be called from the IO thread
    // so make sure it doesn't touch any QObject things
    virtual Tag readTag() = 0;

    Tag createTag();
    Tag createTag(const QString& filename, const QHash<QString, QVariant>& data);
    QString filename() const;

private:
    QString m_filename;

    friend class TagJob;
};

#endif
