#include "updater.h"
#include "awsconfig.h"
#include <QApplication>
#include <QFileDialog>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    if (!AwsConfig::init())
        return 1;

    QString dir = QFileDialog::getExistingDirectory();
    if (dir.isEmpty())
        return 0;

    Updater updater;
    updater.update(dir);

    return app.exec();
}
