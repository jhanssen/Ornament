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

    bool isValid() const;

private:
#ifdef BUILDING_UPDATER
public:
#endif
    Tag(const QString& filename);

    QString m_filename;
    QHash<QString, QVariant> m_data;

    friend class MediaJob;
};

Q_DECLARE_METATYPE(Tag)

#endif
