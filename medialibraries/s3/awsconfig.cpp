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

#include "awsconfig.h"
#include <QFile>
#include <QDir>
#include <QDebug>

QList<QByteArray> AwsConfig::s_items;

AwsConfig::AwsConfig()
{
}

bool AwsConfig::init()
{
    QDir home = QDir::home();
    QFile cfg(home.absoluteFilePath(".player-aws"));
    if (!cfg.open(QFile::ReadOnly))
        return false;

    QByteArray data = cfg.readAll();
    s_items = data.split('\n');

    return (s_items.size() >= 3);
}

const char* AwsConfig::accessKey()
{
    if (s_items.size() < 1)
        return 0;
    return s_items.at(0).constData();
}

const char* AwsConfig::secretKey()
{
    if (s_items.size() < 2)
        return 0;
    return s_items.at(1).constData();
}

const char* AwsConfig::bucket()
{
    if (s_items.size() < 3)
        return 0;
    return s_items.at(2).constData();
}
