#include "audiodevice.h"
#include "audioplayer.h"
#include "musicmodel.h"
#include "io.h"
#include "filereader.h"

#include <QApplication>
#include <QDeclarativeComponent>
#include <QDeclarativeView>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    IO::init();
    FileReader::registerType();

    qmlRegisterType<AudioDevice>("AudioDevice", 1, 0, "AudioDevice");
    qmlRegisterType<AudioPlayer>("AudioPlayer", 1, 0, "AudioPlayer");
    qmlRegisterType<MusicModel>("MusicModel", 1, 0, "MusicModel");

    QDeclarativeView view(QUrl::fromLocalFile("player.qml"));
    view.show();

    return app.exec();
}
