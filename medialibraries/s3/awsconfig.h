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

#ifndef AWSCONFIG_H
#define AWSCONFIG_H

#include <QList>
#include <QByteArray>

class AwsConfig
{
public:
    static bool init();

    static const char* accessKey();
    static const char* secretKey();
    static const char* bucket();

private:
    AwsConfig();

    static QList<QByteArray> s_items;
};

#endif // AWSCONFIG_H
