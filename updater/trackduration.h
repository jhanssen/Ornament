#ifndef TRACKDURATION_H
#define TRACKDURATION_H

#include <QFileInfo>

class TrackDuration
{
public:
    static int duration(const QFileInfo& fileinfo);

private:
    TrackDuration();
};

#endif // TRACKDURATION_H
