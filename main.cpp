#include "audiodevice.h"
#include "audioplayer.h"
#include "musicmodel.h"

#include <QApplication>
#include <QDeclarativeComponent>
#include <QDeclarativeView>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    /*
    AudioDevice device;

    QStringList deviceList = device.devices();
    if (deviceList.isEmpty())
        return 1;

    device.setDevice(deviceList.at(0));


    AudioPlayer player;

    player.setAudioDevice(&device);
    player.setFilename("/Users/jhanssen/mp3/Bonobo/Animal Magic/02 - Sleepy Seven.mp3");

    player.play();
    */

    qmlRegisterType<AudioDevice>("AudioDevice", 1, 0, "AudioDevice");
    qmlRegisterType<AudioPlayer>("AudioPlayer", 1, 0, "AudioPlayer");
    qmlRegisterType<MusicModel>("MusicModel", 1, 0, "MusicModel");

    QDeclarativeView view(QUrl::fromLocalFile("player.qml"));
    view.show();

    return app.exec();
}
