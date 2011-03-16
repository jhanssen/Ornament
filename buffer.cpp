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
