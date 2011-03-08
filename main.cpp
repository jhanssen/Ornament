#include <QtGui>
#include <QtMultimedia>
#include "codecs/codecs.h"
#include "codecdevice.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    Codecs::init();

    Codec* codec = Codecs::create(QLatin1String("audio/mp3"));
    if (!codec)
        return 1;

    QAudioOutput* output = 0;
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    foreach(QAudioDeviceInfo device, devices) {
        qDebug() << device.deviceName();
        if (output == 0)
            output = new QAudioOutput(device, device.preferredFormat());
    }

    if (!output) {
        qDebug() << "no output device";
        return 1;
    }

    if (output->error() != QAudio::NoError) {
        qDebug() << "error" << output->error();
        return 1;
    }

    codec->init(output->format());

    QFile inputfile("/Users/jhanssen/mp3/Bonobo/Animal Magic/02 - Sleepy Seven.mp3");
    if (!inputfile.open(QFile::ReadOnly))
        return 2;

    CodecDevice* cd = new CodecDevice;
    cd->setInputDevice(&inputfile);
    cd->setCodec(codec);
    cd->open(QIODevice::ReadOnly);

    output->start(cd);

    return app.exec();
}
