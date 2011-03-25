#include "audiodevice.h"
#include "audioplayer.h"
#include "musicmodel.h"
#include "io.h"
#include "codecs/codecs.h"
#include "medialibrary.h"

#include <QApplication>
#include <QDeclarativeComponent>
#include <QDeclarativeView>
#include <QDeclarativeEngine>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    IO::init();
    Codecs::init();
    MediaLibrary::init();

    QSettings settings(QLatin1String("hepp"), QLatin1String("player"));
    MediaLibrary::instance()->setSettings(&settings);

    qmlRegisterType<AudioDevice>("AudioDevice", 1, 0, "AudioDevice");
    qmlRegisterType<AudioPlayer>("AudioPlayer", 1, 0, "AudioPlayer");
    qmlRegisterType<MusicModel>("MusicModel", 1, 0, "MusicModel");
    qmlRegisterType<MediaModel>("MediaModel", 1, 0, "MediaModel");

    QDeclarativeView view(QUrl::fromLocalFile("player.qml"));

    view.engine()->addImageProvider(QLatin1String("artwork"), new AudioImageProvider);

    view.setResizeMode(QDeclarativeView::SizeRootObjectToView);
    view.show();

    int r = app.exec();

    delete MediaLibrary::instance();

    return r;
}
