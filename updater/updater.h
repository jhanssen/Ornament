#ifndef UPDATER_H
#define UPDATER_H

#include <QObject>
#include <QList>
#include <QFileInfo>

class Progress;

class Updater : public QObject
{
    Q_OBJECT
public:
    Updater(QObject *parent = 0);
    ~Updater();

    void update(const QString& path);

    int readFromCurrent(int bufferSize, char* buffer);
    QString currentFilename() const;

    qint64 currentPosition() const;
    qint64 currentSize() const;

    void updateProgress(int read);
    void updateProgressName(const QString& artist, const QString& album, const QString& track);

private slots:
    void startUpdate();
    void updateDirectory(const QString& path);

private:
    QString m_path;
    QList<QFileInfo> m_update;
    quint64 m_totalsize;

    bool m_writingArtwork;
    QFile* m_current;

    Progress* m_progress;
};

#endif // UPDATER_H
