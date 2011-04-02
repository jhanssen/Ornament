#ifndef AUDIOREADER_H
#define AUDIOREADER_H

#include <QIODevice>

class AudioReader : public QIODevice
{
    Q_OBJECT
public:
    AudioReader(QObject *parent = 0);

    virtual void pause();
    virtual void resume();
};

#endif // AUDIOREADER_H
