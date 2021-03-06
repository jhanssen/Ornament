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

#include "audiodevice.h"
#include "audioplayer.h"
#include "musicmodel.h"
#include "io.h"
#include "codecs/codecs.h"
#include "medialibrary_file.h"
#include "medialibrary_s3.h"
#include "awsconfig.h"

#include <QApplication>
#include <QDeclarativeComponent>
#include <QDeclarativeView>
#include <QDeclarativeEngine>

class MainView : public QDeclarativeView
{
public:
    MainView(const QUrl& url, QWidget* parent = 0);

protected:
    void showEvent(QShowEvent* event);
};

MainView::MainView(const QUrl& url, QWidget* parent)
    : QDeclarativeView(url, parent)
{
}

void MainView::showEvent(QShowEvent* event)
{
    raise();

    QDeclarativeView::showEvent(event);
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    bool s3 = false;
    for (int i = 1; i < argc; ++i) {
        if (qstrcmp(argv[i], "-s3") == 0)
            s3 = true;
    }

    if (s3 && !AwsConfig::init()) {
        qDebug() << "unable to read aws config from ~/.player-aws";
        return 1;
    }

    IO::init();
    Codecs::init();
    if (s3)
        MediaLibraryS3::init();
    else
        MediaLibraryFile::init();

    QSettings settings(QLatin1String("hepp"), QLatin1String("player"));
    MediaLibrary::instance()->setSettings(&settings);

    qmlRegisterType<AudioDevice>("AudioDevice", 1, 0, "AudioDevice");
    qmlRegisterType<AudioPlayer>("AudioPlayer", 1, 0, "AudioPlayer");
    qmlRegisterType<MusicModel>("MusicModel", 1, 0, "MusicModel");
    qmlRegisterType<MediaModel>("MediaModel", 1, 0, "MediaModel");

    MainView view(QUrl::fromLocalFile("player.qml"));

    view.engine()->addImageProvider(QLatin1String("artwork"), new AudioImageProvider);

    view.setResizeMode(QDeclarativeView::SizeRootObjectToView);
    view.show();

    int r = app.exec();

    delete MediaLibrary::instance();

    return r;
}
