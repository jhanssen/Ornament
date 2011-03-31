#include "awsconfig.h"
#include <QFile>
#include <QDir>
#include <QDebug>

QList<QByteArray> AwsConfig::s_items;

AwsConfig::AwsConfig()
{
}

bool AwsConfig::init()
{
    QDir home = QDir::home();
    QFile cfg(home.absoluteFilePath(".player-aws"));
    if (!cfg.open(QFile::ReadOnly))
        return false;

    QByteArray data = cfg.readAll();
    s_items = data.split('\n');

    return (s_items.size() >= 3);
}

const char* AwsConfig::accessKey()
{
    if (s_items.size() < 1)
        return 0;
    return s_items.at(0).constData();
}

const char* AwsConfig::secretKey()
{
    if (s_items.size() < 2)
        return 0;
    return s_items.at(1).constData();
}

const char* AwsConfig::bucket()
{
    if (s_items.size() < 3)
        return 0;
    return s_items.at(2).constData();
}
