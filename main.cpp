#include <QtGui>
#include "audiodevice.h"
#include "audioplayer.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    AudioDevice device;

    QStringList deviceList = device.devices();
    if (deviceList.isEmpty())
        return 1;

    device.setDevice(deviceList.at(0));


    AudioPlayer player;

    player.setAudioDevice(&device);
    player.setFilename("/Users/jhanssen/mp3/Bonobo/Animal Magic/02 - Sleepy Seven.mp3");

    player.play();

    return app.exec();
}
