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

#include "buffer.h"
#include <QtAlgorithms>

Buffer::Buffer()
    : m_size(0)
{
}

Buffer::~Buffer()
{
    qDeleteAll(m_subs);
}

void Buffer::add(QByteArray *sub)
{
    m_subs.append(sub);
    m_size += sub->size();
}

int Buffer::size() const
{
    return m_size;
}

bool Buffer::isEmpty() const
{
    return m_size == 0;
}

void Buffer::clear()
{
    qDeleteAll(m_subs);
    m_subs.clear();
    m_size = 0;
}

QByteArray Buffer::read(int size)
{
    QByteArray ret;
    QByteArray* cur;
    QLinkedList<QByteArray*>::Iterator it = m_subs.begin();
    while (it != m_subs.end()) {
        cur = *it;
        if (ret.size() + cur->size() <= size)
            ret += *cur;
        else {
            int rem = size - ret.size();
            ret += cur->left(rem);
            *cur = cur->mid(rem);
            break;
        }
        delete cur;
        it = m_subs.erase(it);
    }
    m_size -= ret.size();
    return ret;
}
