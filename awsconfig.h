#ifndef AWSCONFIG_H
#define AWSCONFIG_H

#include <QList>
#include <QByteArray>

class AwsConfig
{
public:
    static bool init();

    static const char* accessKey();
    static const char* secretKey();
    static const char* bucket();

private:
    AwsConfig();

    static QList<QByteArray> s_items;
};

#endif // AWSCONFIG_H
