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
