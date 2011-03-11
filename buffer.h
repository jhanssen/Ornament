#ifndef BUFFER_H
#define BUFFER_H

#include <QByteArray>
#include <QLinkedList>

class Buffer
{
public:
    Buffer();

    void add(QByteArray* sub);
    void clear();

    bool isEmpty() const;
    int size() const;
    QByteArray read(int size);

private:
    QLinkedList<QByteArray*> m_subs;
    int m_size;
};

#endif // BUFFER_H
